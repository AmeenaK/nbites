#include <Python.h>
#include <exception>
#include <boost/shared_ptr.hpp>

#include "Noggin.h"
#include "nogginconfig.h"
#include "PyLoc.h"
#include "EKFStructs.h"
#include "_ledsmodule.h"
#include "PySensors.h"
#include "PyRoboGuardian.h"
#include "PyMotion.h"

//#define DEBUG_OBSERVATIONS
//#define DEBUG_BALL_OBSERVATIONS
#define RUN_LOCALIZATION
#define USE_LOC_CORNERS
static const float MAX_CORNER_DISTANCE = 150.0f;
using namespace std;
using namespace boost;

const char * BRAIN_MODULE = "man.noggin.Brain";
const int TEAMMATE_FRAMES_OFF_THRESH = 5;
Noggin::Noggin (shared_ptr<Profiler> p, shared_ptr<Vision> v,
                shared_ptr<Comm> c, shared_ptr<RoboGuardian> rbg,
                MotionInterface * _minterface)
    : comm(c),gc(c->getGC()),
      chestButton(rbg->getButton(CHEST_BUTTON)),
      leftFootButton(rbg->getButton(LEFT_FOOT_BUTTON)),
      rightFootButton(rbg->getButton(RIGHT_FOOT_BUTTON)),
      error_state(false), brain_module(NULL), brain_instance(NULL),
      motion_interface(_minterface),registeredGCReset(false), ballFramesOff(0),
      do_reload(0)
{
#ifdef DEBUG_NOGGIN_INITIALIZATION
    printf("Noggin::initializing\n");
#endif

    // Initialize the interpreter and C python extensions
    initializePython(v);

    // import noggin.Brain and instantiate a Brain reference
    import_modules();

#ifdef DEBUG_NOGGIN_INITIALIZATION
    printf("  Retrieving Brain.Brain instance\n");
#endif

    // Instantiate a Brain instance
    getBrainInstance();

#ifdef DEBUG_NOGGIN_INITIALIZATION
    printf("  DONE!\n");
#endif
}

Noggin::~Noggin ()
{
    Py_XDECREF(brain_instance);
    Py_XDECREF(brain_module);
}

void Noggin::initializePython(shared_ptr<Vision> v)
{
#ifdef DEBUG_NOGGIN_INITIALIZATION
    printf("  Initializing interpreter and extension modules\n");
#endif

    // Initialize low-level modules
    c_init_sensors();
    init_leds();
    c_init_roboguardian();
    c_init_motion();
    c_init_comm();
    comm->add_to_module();

    // Initialize PyVision module
    vision = v;
    MODULE_INIT(vision) ();

    // Initialize and insert the vision wrapper into the module
    PyObject *result = PyVision_new(v.get());
    if (result == NULL) {
        cerr << "** Noggin extension could not initialize PyVision object **" <<
            endl;
        assert(false);
    }
    vision_addToModule(result, MODULE_HEAD);
    pyvision = reinterpret_cast<PyVision*>(result);

    // Initialize localization stuff
    initializeLocalization();

}

void Noggin::initializeLocalization()
{
#ifdef DEBUG_NOGGIN_INITIALIZATION
    printf("Initializing localization modules\n");
#endif

    // Initialize the localization modules
    //loc = shared_ptr<MCL>(new MCL());
    loc = shared_ptr<LocEKF>(new LocEKF());
    ballEKF = shared_ptr<BallEKF>(new BallEKF());

    // Setup the python localization wrappers
    set_loc_reference(loc);
    set_ballEKF_reference(ballEKF);
    c_init_localization();

    // Set the comm localization access pointers
    comm->setLocalizationAccess(loc, ballEKF);
}

bool Noggin::import_modules ()
{
#ifdef  DEBUG_NOGGIN_INITIALIZATION
    printf("  Importing noggin.Brain\n");
#endif

    // Load Brain module
    //
    if (brain_module == NULL)
        // Import brain module
        brain_module = PyImport_ImportModule(BRAIN_MODULE);

    if (brain_module == NULL) {
        // error, couldn't import noggin.Brain
        fprintf(stderr, "Error importing noggin.Brain module\n");
        if (PyErr_Occurred())
            PyErr_Print();
        else
            fprintf(stderr, "  No Python exception information available\n");
        return false;
    }

    return true;
}

void Noggin::reload_hard ()
{
    // finalize and reinitialize the Python interpreter
    Py_Finalize();
    Py_Initialize();

    // load C extension modules
    initializePython(vision);
}

void Noggin::reload_brain ()
{
    if (brain_module == NULL)
        if (!import_modules())
            return;

    // reload Brain module
    PyImport_ReloadModule(brain_module);
    // Instantiate a Brain instance
    getBrainInstance();
}

void Noggin::reload_modules(std::string modules)
{
    if (brain_module == NULL)
        if (!import_modules())
            return;

    /* Not currently implemented */

    // reload Brain module
    PyImport_ReloadModule(brain_module);
    // Instantiate a Brain instance
    getBrainInstance();
}

void Noggin::getBrainInstance ()
{
    if (brain_module == NULL)
        if (!import_modules())
            return;

    // drop old reference
    Py_XDECREF(brain_instance);
    // Grab instantiate and hold a reference to a new noggin.Brain.Brain()
    PyObject *dict = PyModule_GetDict(brain_module);
    PyObject *brain_class = PyDict_GetItemString(dict, "Brain");
    if (brain_class != NULL)
        brain_instance = PyObject_CallObject(brain_class, NULL);
    else
        brain_instance = NULL;

    if (brain_instance == NULL) {
        fprintf(stderr, "Error accessing noggin.Brain.Brain\n");
        if (PyErr_Occurred())
            PyErr_Print();
        else
            fprintf(stderr, "  No error available\n");
    }

    // Successfully reloaded
    error_state = (brain_instance == NULL);
}

void Noggin::runStep ()
{
#ifdef USE_NOGGIN_AUTO_HALT
    // don't bother doing anything if there's a Python error and we
    // haven't reloaded
    if (error_state)
        return;
#endif


    //Check button pushes for game controller signals
    processGCButtonClicks();

    PROF_ENTER(profiler, P_PYTHON);

    // Update vision information for Python
    PROF_ENTER(profiler, P_PYUPDATE);
    PyVision_update(pyvision);
    PROF_EXIT(profiler, P_PYUPDATE);

#ifdef RUN_LOCALIZATION
    // Update localization information
    PROF_ENTER(profiler, P_LOC);
    updateLocalization();
    PROF_EXIT(profiler, P_LOC);
#endif //RUN_LOCALIZATION

    // Call main run() method of Brain
    PROF_ENTER(profiler, P_PYRUN);
    if (brain_instance != NULL) {
        PyObject *result = PyObject_CallMethod(brain_instance, "run", NULL);
        if (result == NULL) {
            // set Noggin in error state
            error_state = true;
            // report error
            fprintf(stderr, "Error occurred in noggin.Brain.run() method\n");
            if (PyErr_Occurred()) {
                PyErr_Print();
            } else {
                fprintf(stderr,
                        "  No Python exception information available\n");
            }
        } else {
            Py_DECREF(result);
        }
    }
    PROF_EXIT(profiler, P_PYRUN);

    PROF_EXIT(profiler, P_PYTHON);
}

void Noggin::updateLocalization()
{
    // Self Localization
    //MotionModel odometery(0.0f, 0.0f, 0.0f);
    MotionModel odometery = motion_interface->getOdometryUpdate();

    // Build the observations from vision data
    vector<Observation> observations;
    // FieldObjects
    VisualFieldObject fo;
    fo = *vision->bgrp;

    if(fo.getDistance() > 0 && fo.getDistanceCertainty() != BOTH_UNSURE) {
        Observation seen(fo);
        observations.push_back(seen);
#ifdef DEBUG_OBSERVATIONS
        cout << "Saw bgrp at distance " << fo.getDistance()
             << " and bearing " << seen.getVisBearing() << endl;
#endif
    }

    fo = *vision->bglp;
    if(fo.getDistance() > 0 && fo.getDistanceCertainty() != BOTH_UNSURE) {
        Observation seen(fo);
        observations.push_back(seen);
#ifdef DEBUG_OBSERVATIONS
        cout << "Saw bglp at distance " << fo.getDistance()
             << " and bearing " << seen.getVisBearing() << endl;
#endif
    }

    fo = *vision->ygrp;
    if(fo.getDistance() > 0 && fo.getDistanceCertainty() != BOTH_UNSURE) {
        Observation seen(fo);
        observations.push_back(seen);
#ifdef DEBUG_OBSERVATIONS
        cout << "Saw ygrp at distance " << fo.getDistance()
             << " and bearing " << seen.getVisBearing() << endl;
#endif
    }

    fo = *vision->yglp;
    if(fo.getDistance() > 0 && fo.getDistanceCertainty() != BOTH_UNSURE) {
        Observation seen(fo);
        observations.push_back(seen);
#ifdef DEBUG_OBSERVATIONS
        cout << "Saw yglp at distance " << fo.getDistance()
             << " and bearing " << seen.getVisBearing() << endl;
#endif
    }

    // Corners
#ifdef USE_LOC_CORNERS
    const list<VisualCorner> * corners = vision->fieldLines->getCorners();
    list <VisualCorner>::const_iterator i;
    for ( i = corners->begin(); i != corners->end(); ++i) {
        if (i->getDistance() < MAX_CORNER_DISTANCE) {
            Observation seen(*i);
            observations.push_back(seen);
#           ifdef DEBUG_OBSERVATIONS
            cout << "Saw corner " << i->getID() << " at distance "
                 << seen.getVisDistance() << " and bearing " << seen.getVisBearing()
                 << endl;
#            endif
        }
    }
#endif

    // Lines
    // const vector<VisualLine> * lines = vision->fieldLines->getLines();
    // vector <VisualLine>::const_iterator j;
    // for ( j = lines->begin(); j != lines->end(); ++j) {
    //     Observation seen(*j);
    //     observations.push_back(seen);
    // }

    // Process the information
    PROF_ENTER(profiler, P_MCL);
    loc->updateLocalization(odometery, observations);
    PROF_EXIT(profiler, P_MCL);

    // Ball Tracking
    if (vision->ball->getDistance() > 0.0) {
        ballFramesOff = 0;
    } else {
        ++ballFramesOff;
    }

    if( ballFramesOff < TEAMMATE_FRAMES_OFF_THRESH) {
        // If it's less than the threshold then we either see a ball or report
        // no ball seen
        RangeBearingMeasurement m(vision->ball);
        ballEKF->updateModel(m, loc->getCurrentEstimate());
    } else {
        // If it's off for more then the threshold, then try and use mate data
        RangeBearingMeasurement m;
        // HUGE HACK!!!!
        // This returns the ball x, y, which need to be converted to dist and
        // bearing
        // m = comm->getTeammateBallReport();

        // if (!(m.distance == 0.0 && m.bearing == 0.0) &&
        //     !(gc->gameState() == STATE_INITIAL ||
        //       gc->gameState() == STATE_FINISHED)) {
        //     float ballX = m.distance;
        //     float ballY = m.bearing;
        //     float ballXUncert = m.distanceSD;
        //     float ballYUncert = m.bearingSD;
        //     m.distance = hypot(loc->getXEst() - ballX, loc->getYEst() - ballY);
        //     m.bearing = subPIAngle(atan2(ballY - loc->getYEst(), ballX -
        //                                  loc->getXEst()) - loc->getHEst());
        //     m.distanceSD = vision->ball->ballDistanceToSD(m.distance);
        //     m.bearingSD =  vision->ball->ballBearingToSD(m.bearing);
        //     // cout << "\t\tUsing teammate ball report of (" << m.distance << ", "
        //     //      << m.bearing << ")" << endl;
        // }

        ballEKF->updateModel(m, loc->getCurrentEstimate());
    }

#ifdef DEBUG_BALL_OBSERVATIONS
    if(vision->ball->getDistance() > 0.0) {
        cout << "Ball seen at distance " << vision->ball->getDistance()
             << " and bearing " << vision->ball->getBearing() << endl;
    }
#endif

    // Opponent Tracking

#ifdef DEBUG_OBSERVATIONS
    //cout << *loc << endl;
#endif
}


//#define DEBUG_NOGGIN_GC
void Noggin::processGCButtonClicks(){
    static const int ADVANCE_STATES_CLICKS  = 1;
    static const int SWITCH_TEAM_CLICKS  = 1;
    static const int SWITCH_KICKOFF_CLICKS  = 1;
    static const int REVERT_TO_INITIAL_CLICKS = 4;
    //cout << "In noggin chest clicks are " << chestButton->peekNumClicks() <<endl;

    if(chestButton->peekNumClicks() ==  ADVANCE_STATES_CLICKS){
        gc->advanceOneState();
        chestButton->getAndClearNumClicks();
#ifdef DEBUG_NOGGIN_GC
        cout << "Button pushing advanced GC to state : " << gc->gameState() <<endl;
#endif
    }
    //Only toggle colors and kickoff when you are in initial
    if(gc->gameState() == STATE_INITIAL){
        if(leftFootButton->peekNumClicks() ==  SWITCH_TEAM_CLICKS){
            gc->toggleTeamColor();
            leftFootButton->getAndClearNumClicks();
#ifdef DEBUG_NOGGIN_GC
            cout << "Button pushing switched GC to color : ";
            if(gc->color() == TEAM_BLUE)
                cout << "BLUE" <<endl;
            else
                cout << "RED" <<endl;

#endif
        }

        if(rightFootButton->peekNumClicks() ==  SWITCH_KICKOFF_CLICKS){
            gc->toggleKickoff();
            rightFootButton->getAndClearNumClicks();
#ifdef DEBUG_NOGGIN_GC
            cout << " Button pushing switched GC to kickoff state : ";
            if(gc->kickOffTeam() == gc->team())
                cout << "ON KICKOFF" <<endl;
            else
                cout << "OFF KICKOFF" <<endl;
#endif
        }
    }
    if( chestButton->peekNumClicks() == REVERT_TO_INITIAL_CLICKS){
#ifdef DEBUG_NOGGIN_CC
           cout << "SENDING GC TO INITIAL DUE TO CHEST BUTTONS"
                <<endl;
#endif
           chestButton->getAndClearNumClicks();
           gc->setGameState(STATE_INITIAL);
    }

}
