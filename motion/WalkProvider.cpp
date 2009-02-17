
#include <boost/shared_ptr.hpp>
using boost::shared_ptr;

#include "Sensors.h"
#include "WalkProvider.h"
using Kinematics::LLEG_CHAIN;
using Kinematics::RLEG_CHAIN;

WalkProvider::WalkProvider(shared_ptr<Sensors> s)
    : MotionProvider("WalkProvider"),
      sensors(s),
      walkParameters(.02f,         // motion frame length - FIXME constant
                     310.0f,       // COM height
                     19.0f,        // hipOffsetX
                     0.5f,        // stepDuration
                     0.1f,         // fraction in double support mode
                     16.5f,        // stepHeight
                     10.0f,         // footLengthX
                     0.4f,        // zmp static percentage in double support
                     4.0f*TO_RAD,  // leftSwingHipRollAddition
                     4.0f*TO_RAD,  // rightSwingHipRollAddition
                     12.0f,        // leftZMPSwingOffestY,
                     12.0f),       // rightZMPSwingOffestY
      stepGenerator(sensors,&walkParameters),
      pendingCommands(false),
      nextCommand(NULL)
{
    pthread_mutex_init(&walk_command_mutex,NULL);

    //Make up something arbitrary for the arms
    const float larm[ARM_JOINTS] = {M_PI/2,M_PI/10,-M_PI/2,-M_PI/2};
    const float rarm[ARM_JOINTS] = {M_PI/2,-M_PI/10,M_PI/2,M_PI/2};
    larm_angles = vector<float>(larm,larm+ARM_JOINTS);
    rarm_angles = vector<float>(rarm,rarm+ARM_JOINTS);

    setActive();
}

WalkProvider::~WalkProvider() {
    pthread_mutex_destroy(&walk_command_mutex);
}

void WalkProvider::requestStopFirstInstance() {
    cout << "REQUEST STOP FIRST INSTANCE" <<endl;
    setCommand(new WalkCommand(0.0f, 0.0f, 0.0f));
}

void WalkProvider::calculateNextJoints() {
    pthread_mutex_lock(&walk_command_mutex);
    if(nextCommand){
        stepGenerator.setSpeed(nextCommand->x_mms,
                               nextCommand->y_mms,
                               nextCommand->theta_rads);
    cout << "Finished sending the command to step generator" <<endl;
    }
    pendingCommands = false;
    nextCommand = NULL;
    pthread_mutex_unlock(&walk_command_mutex);


    //ask the step Generator to update ZMP values, com targets
    stepGenerator.tick_controller();

    // Now ask the step generator to get the leg angles
    WalkLegsTuple legs_result = stepGenerator.tick_legs();

    //Get the joints for each Leg
    vector<float> lleg_results = legs_result.get<LEFT_FOOT>();
    vector<float> rleg_results = legs_result.get<RIGHT_FOOT>();

    // vector<float> rarm_results = vector<float>(ARM_JOINTS, 0.0f);
//     vector<float> larm_results = vector<float>(ARM_JOINTS, 0.0f);

//     cout << "2rarm size "<< rarm_results.size()
//          << "larm size "<< larm_results.size() <<endl;


    //Return the joints for the legs
    setNextChainJoints(LARM_CHAIN,larm_angles);
    setNextChainJoints(LLEG_CHAIN,lleg_results);
    setNextChainJoints(RLEG_CHAIN,rleg_results);
    setNextChainJoints(RARM_CHAIN,rarm_angles);

    setActive();
}

void WalkProvider::setCommand(const WalkCommand * command){
    //grab the velocities in mm/second rad/second from WalkCommand
    cout << "Got walk command in walk Provider" << *command<<endl;
    pthread_mutex_lock(&walk_command_mutex);
    if(nextCommand)
        delete nextCommand;
    nextCommand =command;
    pendingCommands = true;
    pthread_mutex_unlock(&walk_command_mutex);

    setActive();
}

//Returns the 20 body joints
vector<float> WalkProvider::getWalkStance(){
    //cout << "getWalkStance" <<endl;
    //calculate the walking stance of the robot
    const float z = walkParameters.bodyHeight;
    const float x = walkParameters.hipOffsetX;
    const float ly = HIP_OFFSET_Y;
    const float ry = -HIP_OFFSET_Y;


    //just assume we start at zero
    float zeroJoints[LEG_JOINTS] = {0.0f,0.0f,0.0f,
                                    0.0f,0.0f,0.0f};
    //Use inverse kinematics to find the left leg angles
    ufvector3 lgoal = ufvector3(3);
    lgoal(0)=-x; lgoal(1) = ly; lgoal(2) = -z;
    IKLegResult lresult = Kinematics::dls(LLEG_CHAIN,lgoal,zeroJoints);
    vector<float> lleg_angles(lresult.angles, lresult.angles + LEG_JOINTS);

    //Use inverse kinematics to find the right leg angles
    ufvector3 rgoal = ufvector3(3);
    rgoal(0)=-x; rgoal(1) = ry; rgoal(2) = -z;
    IKLegResult rresult = Kinematics::dls(RLEG_CHAIN,rgoal,zeroJoints);
    vector<float> rleg_angles(rresult.angles, rresult.angles + LEG_JOINTS);

    vector<float> allJoints;

    //now combine all the vectors together
    allJoints.insert(allJoints.end(),larm_angles.begin(),larm_angles.end());
    allJoints.insert(allJoints.end(),lleg_angles.begin(),lleg_angles.end());
    allJoints.insert(allJoints.end(),rleg_angles.begin(),rleg_angles.end());
    allJoints.insert(allJoints.end(),rarm_angles.begin(),rarm_angles.end());
    return allJoints;
}

void WalkProvider::setActive(){
    //check to see if the walk engine is active
    if(stepGenerator.isDone() && !pendingCommands){
        inactive();
    }else{
        active();
    }
}
