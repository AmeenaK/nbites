/* LocLogFaker.cpp */

/**
 * Format of the log file:
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
 * LANDMARK INFO
 * ID dist bearing (for all landmarks observed in the frame)
 */
#include "LocLogFaker.h"

int main()
{
    // TODO: Make this read a text file
    // Make navPath
    NavPath letsGo;
    letsGo.startPos = PoseEst(200.0f,300.2f,0.0f);
    letsGo.myMoves.push_back(NavMove(MotionModel(0.0f,0.0f,0.0f),
                                     200));
    letsGo.myMoves.push_back(NavMove(MotionModel(2.0f,0.0f,0.0f),
                                     100));
    letsGo.myMoves.push_back(NavMove(MotionModel(-2.0f,0.0f,0.0f),
                                     200));
    letsGo.myMoves.push_back(NavMove(MotionModel(0.0f,3.0f,0.0f),
                                     100));
    letsGo.myMoves.push_back(NavMove(MotionModel(0.0f,-3.0f,0.0f),
                                     125));
    letsGo.myMoves.push_back(NavMove(MotionModel(0.0f,0.0f,-0.0314f),
                                     300));
    letsGo.myMoves.push_back(NavMove(MotionModel(0.0f,0.0f,0.0f),
                                     200));

    // Information needed for the main method
    PoseEst currentPose = letsGo.startPos;
    MCL *myLoc = new MCL;
    vector<Observation> Z_t;
    team_color = "0";
    player_number = "3";

    // Setup output file
    fstream outputFile;
    string outfileName = "FAKELOG.mcl";
    outputFile.open(outfileName.c_str(), ios::out);
    MotionModel noMove(0.0,0.0,0.0);
    printOutLogLine(&outputFile, myLoc, Z_t, noMove);
    bool resample = true;
    // Iterate through the moves
    for(unsigned int i = 0; i < letsGo.myMoves.size(); ++i) {
        // Continue the move for as long as specified
        for (int j = 0; j < letsGo.myMoves[i].time; ++j) {
            currentPose += letsGo.myMoves[i].move;
            if ( (i+j) % RESAMPLE_RATE == 0) {
                resample = true;
                // cout << "Resample set to true" << endl;
            } else {
                resample = false;
                // cout << "Resample set to false" << endl;
            }
            Z_t = determineObservedLandmarks(currentPose,0.0);
            myLoc->updateLocalization(letsGo.myMoves[i].move,Z_t, resample);
            printOutLogLine(&outputFile, myLoc, Z_t, letsGo.myMoves[i].move);
        }
    }

    return 0;
}

vector<Observation> determineObservedLandmarks(PoseEst myPos, float neckYaw)
{
    vector<Observation> Z_t;
    // Get half of the nao FOV converted to radians
    float FOV_OFFSET = NAO_FOV_X_DEG * M_PI / 360.0f;

    // Check concrete field objects
    // required measurements for the added observation
    float visDist, visBearing, sigmaD, sigmaB;
    for(int i = 0; i < ConcreteFieldObject::NUM_FIELD_OBJECTS; ++i) {
       const ConcreteFieldObject* toView = ConcreteFieldObject::
           concreteFieldObjectList[i];
       float deltaX = toView->getFieldX() - myPos.x;
        float deltaY = toView->getFieldY() - myPos.y;
        visDist = hypot(deltaX, deltaY);
        visBearing = subPIAngle(atan2(deltaY, deltaX) - myPos.h
                                - (M_PI / 2.0f));

        // Check if the object is viewable
        if (visBearing > -FOV_OFFSET && visBearing < FOV_OFFSET) {

            // Get measurement variance and add noise to reading
            sigmaD = getDistSD(visDist);
            //visDist += sigmaD*UNIFORM_1_NEG_1+.005*sigmaD;
            sigmaB = getBearingSD(visBearing);
            visBearing += (M_PI / 2.0f);
            //visBearing += sigmaB*UNIFORM_1_NEG_1+.005*sigmaB;

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
            // cout << "ID: " << seen.getID() << "\tNumPos: " <<
            //     seen.getNumPossibilities() << endl;
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
            // cout << "Observed landmark: " << toView->getID() << endl;

            // Get measurement variance and add noise to reading
            sigmaD = getDistSD(visDist);
            visDist += sigmaD*UNIFORM_1_NEG_1+.005*sigmaD;
            sigmaB = getBearingSD(visBearing);
            visBearing += (M_PI / 2.0f);
            //visBearing += sigmaB*UNIFORM_1_NEG_1+.005*sigmaB;

            // Ignore the center circle for right now
            if (toView->getID() == CENTER_CIRCLE) {
                continue;
            }
            const cornerID id = toView->getID();
            list <const ConcreteCorner*> toUse;
            // Randomly set ambiguous data
            if ((rand() / (float(RAND_MAX)+1)) < 0.65) {
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
            //cout << "Look at the corner id " << seen.getID() << endl;
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
 * Prints the input to a log file to be read by the TOOL
 */
void printOutLogLine(fstream* outputFile, MCL* myLoc, vector<Observation>
                     sightings, MotionModel lastOdo)
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
                  << "0.0" << " " << "0.0" << " " // Ball x,y
                  << "0.0" << " " << "0.0" << " " // Ball Uncert
                  << "0.0" << " " << "0.0" << " " // Ball Velocity
                  << "0.0" << " " << "0.0" << " " // Ball Vel uncert
                  << lastOdo.deltaL << " " << lastOdo.deltaF << " "
                  << lastOdo.deltaR;

    // Divide the sections with a colon
    (*outputFile) << ":";

    // Output landmark infos
    for(unsigned int k = 0; k < sightings.size(); ++k) {
        (*outputFile) << sightings[k].getID() << " "
                      << sightings[k].getVisDistance() << " "
                      << sightings[k].getVisBearing() << " ";
    }

    // Close the line
    (*outputFile) << endl;
}



// NavMove
// Constructors
NavMove::NavMove(MotionModel _p, int _t) : move(_p), time(_t)
{
};

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
    return distance * 0.15;
}

/**
 * Get standard deviation for the associated bearing reading
 */
float getBearingSD(float bearing)
{
    return bearing*0.12;
}
