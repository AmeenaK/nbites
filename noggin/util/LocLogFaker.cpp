/* LocLogFaker.cpp */

/**
 * Format of the navigation input file (*.nav):
 *
 * START POSITION LINE
 * x-value y-value heading-value
 *
 * NAVIGATION LINES
 * deltaForward deltaLateral deltaRotation
 *
 * Format of the log output file (*.mcl):
 *
 * PARTICLE INFO
 * x y h weight (for M particles)
 *
 * Colon signifying end of section
 *
 * DEBUG INFO
 * team_color player_number
 * x-estimate y-estimate heading-estimate deg
 * x-uncertinty y-uncert heading-uncert
 * ball-x ball-y
 * ball-uncert-x ball-uncert-y
 * ball-vel-x ball-vel-y
 * ball-vel-uncert-x ball-vel-uncert-y
 * odometery-lateral odometery-forward odometery-rotational
 *
 * ROBOT REAL INFO
 * x y h (as given by the system)
 *
 * LANDMARK INFO
 * ID dist bearing (for all landmarks observed in the frame)
 *
 */
#include "LocLogFaker.h"

using namespace std;

int main(int argc, char** argv)
{
    // Information needed for the main method
    // Make navPath
    NavPath letsGo;
    // IO Variables
    fstream inputFile;
    fstream outputFile;

    /* Test for the correct number of CLI arguments */
    if(argc < 2 || argc > 3) {
        cerr << "usage: " << argv[0] << " input-file [output-file]" << endl;
        return 1;
    }
    try {
        inputFile.open(argv[1], ios::in);

    } catch (const exception& e) {
        cout << "Failed to open input file" << argv[1] << endl;
        return 1;
    }

    // Get the info from the file
    readInputFile(&inputFile, &letsGo);

    // Clost the file
    inputFile.close();

    // Open output file
    if(argc > 2) { // If an output file is specified
        outputFile.open(argv[2], ios::out);
    } else { // Otherwise use the default
        outputFile.open(DEFAULT_OUTFILE_NAME.c_str(), ios::out);
    }

    // Iterate through the path
    iteratePath(&outputFile, &letsGo);

    // Close the output file
    outputFile.close();

    return 0;
}

/**
 * Method to iterate through a robot path and write the localization info.
 *
 * @param outputFile The file to have everything printed to
 * @param letsGo The robot path from which to localize
 */
void iteratePath(fstream * outputFile, NavPath * letsGo)
{
    // Method variables
    vector<Observation> Z_t;
    MCL *myLoc = new MCL;
    BallEKF *ballEKF = new BallEKF(myLoc);
    PoseEst currentPose;
    MotionModel *noMove = new MotionModel(0.0, 0.0, 0.0);

    currentPose.x = letsGo->startPos.x;
    currentPose.y = letsGo->startPos.y;
    currentPose.h = letsGo->startPos.h;

    // Print out starting configuration
    printOutLogLine(outputFile, myLoc, Z_t, *noMove, &currentPose,
                    &letsGo->ballStart);
    delete noMove;

    // Iterate through the moves
    for(unsigned int i = 0; i < (*letsGo).myMoves.size(); ++i) {

        // Continue the move for as long as specified
        for (int j = 0; j < (*letsGo).myMoves[i].time; ++j) {
            currentPose += (*letsGo).myMoves[i].move;
            Z_t = determineObservedLandmarks(currentPose, 0.0);
            myLoc->updateLocalization((*letsGo).myMoves[i].move,Z_t);

            // Print the current frame to file
            printOutLogLine(outputFile, myLoc, Z_t, (*letsGo).myMoves[i].move,
                            &currentPose, &letsGo->ballStart);
        }
    }
}

/**
 * Method to determine which landmarks are viewable given a robot pose on the
 * field
 *
 * @param myPos The current pose of the robot
 * @param neckYaw The current head yaw of the robot
 *
 * @return A vector containing all of the observable landmarks at myPose
 */
vector<Observation> determineObservedLandmarks(PoseEst myPos, float neckYaw)
{
    vector<Observation> Z_t;
    // Get half of the nao FOV converted to radians
    float FOV_OFFSET = NAO_FOV_X_DEG * M_PI / 360.0f;
    // Measurements between robot position and seen object
    float deltaX, deltaY;
    // required measurements for the added observation
    float visDist, visBearing, sigmaD, sigmaB;

    // Check concrete field objects
    for(int i = 0; i < ConcreteFieldObject::NUM_FIELD_OBJECTS; ++i) {
        const ConcreteFieldObject* toView = ConcreteFieldObject::
            concreteFieldObjectList[i];
        deltaX = toView->getFieldX() - myPos.x;
        deltaY = toView->getFieldY() - myPos.y;
        visDist = hypot(deltaX, deltaY);
        visBearing = subPIAngle(atan2(deltaY, deltaX) - myPos.h
                                - (M_PI / 2.0f));

        // Check if the object is viewable
        if (visBearing > -FOV_OFFSET && visBearing < FOV_OFFSET) {

            // Get measurement variance and add noise to reading
            sigmaD = getDistSD(visDist);
            visDist += sigmaD*UNIFORM_1_NEG_1;
            sigmaB = getBearingSD(visBearing);
            visBearing += sigmaB*UNIFORM_1_NEG_1;

            // Build the (visual) field object
            fieldObjectID foID = toView->getID();
            VisualFieldObject fo(foID);
            fo.setDistanceWithSD(visDist);
            fo.setBearingWithSD(visBearing);

            // set ambiguous data
            // Randomly set them to abstract
            if ((rand() / (float(RAND_MAX)+1)) < 0.12) {
                fo.setIDCertainty(NOT_SURE);
                if(foID == BLUE_GOAL_LEFT_POST ||
                   foID == BLUE_GOAL_RIGHT_POST) {
                    fo.setID(BLUE_GOAL_POST);
                } else {
                    fo.setID(YELLOW_GOAL_POST);
                }
            } else {
                fo.setIDCertainty(_SURE);
            }
            Observation seen(fo);
            Z_t.push_back(seen);
        }
    }

    // Check concrete corners
    for(int i = 0; i < ConcreteCorner::NUM_CORNERS; ++i) {
       const ConcreteCorner* toView = ConcreteCorner::concreteCornerList[i];
        float deltaX = toView->getFieldX() - myPos.x;
        float deltaY = toView->getFieldY() - myPos.y;
        visDist = hypot(deltaX, deltaY);
        visBearing = subPIAngle(atan2(deltaY, deltaX) - myPos.h
                                - (M_PI / 2.0f));

        // Check if the object is viewable
        if ((visBearing > -FOV_OFFSET && visBearing < FOV_OFFSET) &&
            visDist < CORNER_MAX_VIEW_RANGE) {

            // Get measurement variance and add noise to reading
            sigmaD = getDistSD(visDist);
            visDist += sigmaD*UNIFORM_1_NEG_1+.005*sigmaD;
            sigmaB = getBearingSD(visBearing);
            //visBearing += (M_PI / 2.0f);
            //visBearing += sigmaB*UNIFORM_1_NEG_1+.005*sigmaB;

            // Ignore the center circle for right now
            if (toView->getID() == CENTER_CIRCLE) {
                continue;
            }
            const cornerID id = toView->getID();
            list <const ConcreteCorner*> toUse;
            // Randomly set ambiguous data
            if ((rand() / (float(RAND_MAX)+1)) < 0.32) {
                shape s = ConcreteCorner::inferCornerType(id);
                toUse = ConcreteCorner::getPossibleCorners(s);
            } else {
                const ConcreteCorner * corn;
                switch(id) {
                case BLUE_CORNER_LEFT_L:
                    corn = &ConcreteCorner::blue_corner_left_l;
                    break;
                case BLUE_CORNER_RIGHT_L:
                    corn = &ConcreteCorner::blue_corner_right_l;
                    break;
                case BLUE_GOAL_LEFT_T:
                    corn = &ConcreteCorner::blue_goal_left_t;
                    break;
                case BLUE_GOAL_RIGHT_T:
                    corn = &ConcreteCorner::blue_goal_right_t;
                    break;
                case BLUE_GOAL_LEFT_L:
                    corn = &ConcreteCorner::blue_goal_left_l;
                    break;
                case BLUE_GOAL_RIGHT_L:
                    corn = &ConcreteCorner::blue_goal_right_l;
                    break;
                case CENTER_BY_T:
                    corn = &ConcreteCorner::center_by_t;
                    break;
                case CENTER_YB_T:
                    corn = &ConcreteCorner::center_yb_t;
                    break;
                case YELLOW_CORNER_LEFT_L:
                    corn = &ConcreteCorner::yellow_corner_left_l;
                    break;
                case YELLOW_CORNER_RIGHT_L:
                    corn = &ConcreteCorner::yellow_corner_right_l;
                    break;
                case YELLOW_GOAL_LEFT_T:
                    corn = &ConcreteCorner::yellow_goal_left_t;
                    break;
                case YELLOW_GOAL_RIGHT_T:
                    corn = &ConcreteCorner::yellow_goal_right_t;
                    break;
                case YELLOW_GOAL_LEFT_L:
                    corn = &ConcreteCorner::yellow_goal_left_l;
                    break;
                case YELLOW_GOAL_RIGHT_L:
                    // Intentional fall through
                default:
                    corn = &ConcreteCorner::yellow_goal_right_l;
                    break;
                }
                // Append to the list
                toUse.assign(1,corn);
            }

            // Build the visual corner
            VisualCorner vc(20, 20, visDist,visBearing,
                            VisualLine(), VisualLine(), 10.0f, 10.0f);
            vc.setPossibleCorners(toUse);

            // Set ID
            if (toUse == ConcreteCorner::lCorners) {
                vc.setID(L_INNER_CORNER);
            } else if (toUse == ConcreteCorner::tCorners) {
                vc.setID(T_CORNER);
            } else {
                vc.setID(id);
            }
            // Build the observation
            Observation seen(vc);
            Z_t.push_back(seen);
        }
    }

    // Check concrete lines

    return Z_t;
}

////////////////////////
// File I/O           //
////////////////////////
/**
 * Method to read in a robot path from a formatted file
 *
 * @param inputFile The opened file containing the path information
 * @param letsGo Where the robot path is to be stored
 */
void readInputFile(fstream* inputFile, NavPath * letsGo)
{
    // Method variables
    MotionModel motion;
    BallPose ballMove;
    int time;

    // Read the start info from the first line of the file
    if (!inputFile->eof()) { // start position
        *inputFile >> letsGo->startPos.x >> letsGo->startPos.y
                   >> letsGo->startPos.h // heaading start
                   >> letsGo->ballStart.x >> letsGo->ballStart.y; // Ball info
    }
    letsGo->ballStart.velX = 0.0;
    letsGo->ballStart.velY = 0.0;

    // Convert input value to radians
    letsGo->startPos.h *= DEG_TO_RAD;

    // Build NavMoves from the remaining lines
    while (!inputFile->eof()) {
        *inputFile >> motion.deltaF >> motion.deltaL >> motion.deltaR
                   >> ballMove.velX >> ballMove.velY
                   >> time;

        motion.deltaR *= DEG_TO_RAD;
        letsGo->myMoves.push_back(NavMove(motion, ballMove, time));
    }
}


/**
 * Prints the input to a log file to be read by the TOOL
 *
 * @param outputFile File to write the log line to
 * @param myLoc Current localization module
 * @param sightings Vector of landmark observations
 * @param lastOdo Odometery since previous frame
 */
void printOutLogLine(fstream* outputFile, MCL* myLoc, vector<Observation>
                     sightings, MotionModel lastOdo, PoseEst *currentPose,
                     BallPose * currentBall)
{
    // Output particle infos
    vector<Particle> particles = myLoc->getParticles();
    for(unsigned int j = 0; j < particles.size(); ++j) {
        Particle p = particles[j];
        (*outputFile) << p << " ";
    }

    // Divide the sections with a colon
    (*outputFile) << ":";

    // Output standard infos
    (*outputFile) << team_color<< " " << player_number << " "
                  << myLoc->getXEst() << " " << myLoc->getYEst() << " "
                  << myLoc->getHEstDeg() << " "
                  << myLoc->getXUncert() << " " << myLoc->getYUncert() << " "
                  << myLoc->getHUncertDeg() << " "
        //<< "0.0" << " " << "0.0" << " " // Ball x,y
                  << (*currentBall).x << " "
                  << (*currentBall).y << " "
                  << "0.0" << " " << "0.0" << " " // Ball Uncert
                  << (*currentBall).velX << " "
                  << (*currentBall).velY << " "
        //<< "0.0" << " " << "0.0" << " " // Ball Velocity
        << "0.0" << " " << "0.0" << " " // Ball Vel uncert
                  << lastOdo.deltaL << " " << lastOdo.deltaF << " "
                  << lastOdo.deltaR;

    // Divide the sections with a colon
    (*outputFile) << ":";

    // Print the actual robot position
    (*outputFile) << (*currentPose).x << " "
                  << (*currentPose).y << " "
                  << (*currentPose).h << " "
    // print actual ball position
                  << (*currentBall).x << " "
                  << (*currentBall).y << " "
                  << (*currentBall).velX << " "
                  << (*currentBall).velY << " ";

    // Divide the sections with a colon
    (*outputFile) << ":";

    // Output landmark infos
    for(unsigned int k = 0; k < sightings.size(); ++k) {
        (*outputFile) << sightings[k].getID() << " "
                      << sightings[k].getVisDistance() << " "
                      << sightings[k].getVisBearingDeg() << " ";
    }

    // Close the line
    (*outputFile) << endl;
}



// NavMove
// Constructors
NavMove::NavMove(MotionModel _p, BallPose _b, int _t) : move(_p), ballVel(_b),
                                                        time(_t)
{
};

/**
 * Returns an equivalent angle to the one passed in with value between positive
 * and negative pi.
 *
 * @param theta The angle to be simplified
 *
 * @return The equivalent angle between -pi and pi.
 */
float subPIAngle(float theta)
{
    while( theta > M_PI) {
        theta -= 2.0f*M_PI;
    }

    while( theta < -M_PI) {
        theta += 2.0f*M_PI;
    }
    return theta;
}

/**
 * Get standard deviation for the associated distance reading
 */
float getDistSD(float distance)
{
    return distance * 0.175 + 2;
}

/**
 * Get standard deviation for the associated bearing reading
 */
float getBearingSD(float bearing)
{
    return bearing*0.20 + M_PI / 4.0;
}
