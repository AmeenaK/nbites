
#include <vector>
#include <unistd.h>

#include "CallbackAction.h"

//#define NO_ACTUAL_MOTION
#define DEBUG_INITIALIZATION

//#define DEBUG_TASK_KILLS

using namespace std;

const float MotionCore::GET_UP_BODY_JOINTS[NUM_BODY_JOINTS] =
  { 0.0f, 0.0f, 0.0f, 0.0f,                 // Left Arm
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,     // Left Leg
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,     // Right Leg
    0.0f, 0.0f, 0.0f, 0.0f };               // Right Arm

const float MotionCore::SIT_DOWN_BODY_JOINTS[NUM_BODY_JOINTS] =
  { 80.0f*TO_RAD, 0.0f, 0.0f, 0.0f,                        // Left Arm
    0.0f, 0.0f, -55.0f * TO_RAD,                   // Left Leg
       130.0f * TO_RAD, -75.0f * TO_RAD, 0.0f,
    0.0f, 0.0f, -55.0f * TO_RAD,                   // Right Leg
       130.0f * TO_RAD, -75.0f * TO_RAD, 0.0f,
    80.0f*TO_RAD, 0.0f, 0.0f, 0.0f };                      // Right Arm

const BodyJointCommand MotionCore::
GET_UP(GET_UP_TIME, new vector<float>(GET_UP_BODY_JOINTS,
				      &GET_UP_BODY_JOINTS[NUM_BODY_JOINTS]),
      SMOOTH_INTERPOLATION);

const BodyJointCommand MotionCore::
SIT_DOWN(GET_UP_TIME, new vector<float>(SIT_DOWN_BODY_JOINTS,
					&SIT_DOWN_BODY_JOINTS[NUM_BODY_JOINTS]),
      SMOOTH_INTERPOLATION);

const vector<float> MotionCore::NOT_MOVING = vector<float>(3,0.0f); //zero motion walk vector

MotionCore::MotionCore (Sensors *s)
  : sensors(s),
    bodyQueue(), headQueue(), headScanQueue(), nextWalkCommand(0),
    motionProxy(AL::ALMotionProxy::getInstance()),
    walkIsStopping(false), running(false),
    latestOdometry(vector<float>(3,0.0f)), currentWalkVector(NOT_MOVING),
    odometryRead(false), lastHeadYaw(0.0f), lastHeadPitch(0.0f), headSpeed(0.0f)
{
  pthread_mutex_init (&motion_mutex,  NULL);
  pthread_mutex_init (&odometry_mutex,  NULL);
  pthread_mutex_init (&head_speed_mutex, NULL);

  for (unsigned int i = 0; i < NUM_CHAINS; i++) {
    chainTimeRemaining[i] = 0.0f;
    chainTaskIDs[i] = NO_TASK;
    preemptiveChainTaskIDs[i] = NO_TASK;
  }

  // LHipRoll(°), RHipRoll(°), HipHeight(m), TorsoYOrientation(°)
  //motionProxy->setWalkExtraConfig( 4.5, -4.5, 0.19, 5.0 );
  // StepLength, StepHeight, StepSide, MaxTurn, ZmpOffsetX, ZmpOffsetY
  //motionProxy->setWalkConfig( 0.04, 0.015, 0.04, 0.3, 0.015, 0.025 );

#ifndef NO_ACTUAL_MOTION
  motionProxy->setBodyStiffness(0.85f, 1.0f);
  SleepMs(1000.0f); // wait for the stiffness to kick in
  // cout << "Getting up. " << endl;
  //enqueue(new BodyJointCommand(GET_UP));
  //processCommands();
  //SleepMs(GET_UP_TIME * 1000);
#endif
}

MotionCore::~MotionCore (void)
{
  for (list<const BodyJointCommand*>::iterator i = bodyQueue.begin();
        i != bodyQueue.end(); i++)
    delete *i;

  for (list<const HeadJointCommand*>::iterator i = headQueue.begin();
        i != headQueue.end(); i++)
    delete *i;

  for (list<const HeadScanCommand*>::iterator i = headScanQueue.begin();
       i != headScanQueue.end(); i++)
    delete *i;

  if (nextWalkCommand)
    delete nextWalkCommand;

  stopBodyMoves();
  stopHeadMoves();
  enqueue(new BodyJointCommand(SIT_DOWN));
  cout << "Sitting down" << endl;
  processCommands();
  SleepMs(SIT_DOWN_TIME * 1000);
  motionProxy->setBodyStiffness(0.0f, 1.0f);
  
  pthread_mutex_destroy (&motion_mutex);
  pthread_mutex_destroy (&odometry_mutex);
  pthread_mutex_destroy (&head_speed_mutex);
}

void
MotionCore::enqueueSequence(vector<BodyJointCommand*> &seq)
{
  pthread_mutex_lock(&motion_mutex);
  for (vector<BodyJointCommand*>::iterator i = seq.begin(); i != seq.end(); i++)
    enqueue(*i);
  pthread_mutex_unlock(&motion_mutex);
}
// Begin walking functions
void MotionCore::setWalkConfig ( float pMaxStepLength, float pMaxStepHeight,
				 float pMaxStepSide, float pMaxStepTurn,
				 float pZmpOffsetX, float pZmpOffsetY) {
  motionProxy->setWalkConfig(pMaxStepLength, pMaxStepHeight,
			     pMaxStepSide, pMaxStepTurn,
			     pZmpOffsetX, pZmpOffsetY);
}

void MotionCore::setWalkArmsConfig ( float pShoulderMedian,
				     float pShoulderAmplitude,
				     float pElbowMedian,
				     float pElbowAmplitude) {
  motionProxy->setWalkArmsConfig(pShoulderMedian,
				 pShoulderAmplitude,
				 pElbowMedian,
				 pElbowAmplitude);
}

void MotionCore::setWalkExtraConfig( float pLHipRollBacklashCompensator,
				     float pRHipRollBacklashCompensator,
				     float pHipHeight,
				     float pTorsoYOrientation) {
  motionProxy->setWalkExtraConfig(pLHipRollBacklashCompensator,
				  pRHipRollBacklashCompensator,
				  pHipHeight,
				  pTorsoYOrientation);
}

void MotionCore::setWalkParameters( const WalkParameters& params) {
  params.apply(motionProxy);
}

// Support leg and balance functions
void
MotionCore::setSupportMode( int pSupportMode ){
  motionProxy->setSupportMode( pSupportMode );
}

int
MotionCore::getSupportMode(){
  return motionProxy->getSupportMode();
}

void
MotionCore::setBalanceMode( int pBalanceMode ){
  motionProxy->setBalanceMode( pBalanceMode );
}

int
MotionCore::getBalanceMode(){
  return motionProxy->getBalanceMode();
}

void
MotionCore::stopHeadMoves() {
  if (chainTaskIDs[HEAD_CHAIN] != NO_TASK){
    const bool success = motionProxy->killTask(chainTaskIDs[HEAD_CHAIN]);
    chainTaskIDs[HEAD_CHAIN] = NO_TASK;

#ifdef DEBUG_TASK_KILLS
    if (success)  cout << "Successfully killed head task" << endl;
    else          cout << "Could not kill head task" << endl;
#endif

  }
  if (preemptiveChainTaskIDs[HEAD_CHAIN] != NO_TASK) {
    const bool success = motionProxy->killTask(preemptiveChainTaskIDs[HEAD_CHAIN]);
    preemptiveChainTaskIDs[HEAD_CHAIN] = NO_TASK;

#ifdef DEBUG_TASK_KILLS
    if (success)  cout << "Successfully killed premptive head task" << endl;
    else          cout << "Could not kill head premptive task" << endl;
#endif
  }

  pthread_mutex_lock(&motion_mutex);
  // Clear the head joint command queue
  for (std::list<const HeadJointCommand*>::iterator i = headQueue.begin();
       i != headQueue.end(); ++i) {
    delete *i;
  }
  headQueue.clear();

  // Clear the head joint command queue
  for (std::list<const HeadScanCommand*>::iterator i = headScanQueue.begin();
       i != headScanQueue.end(); ++i) {
    delete *i;
  }
  headScanQueue.clear();
  pthread_mutex_unlock(&motion_mutex);
}

void
MotionCore::stopBodyMoves() {
#ifdef DEBUG_TASK_KILLS
  cout << "stop body now" << endl;
#endif

  for (unsigned int i = 1; i < NUM_CHAINS; i++) {
    if (chainTaskIDs[i] !=  NO_TASK){
      const bool success = motionProxy->killTask(chainTaskIDs[i]);
#ifdef DEBUG_TASK_KILLS
      if (success)  cout << "Successfully killed task" << endl;
      else          cout << "Could not kill task" << endl;
#endif
      bool success2 = motionProxy->killTask(preemptiveChainTaskIDs[i]);
    }
    chainTaskIDs[i] = NO_TASK;
    preemptiveChainTaskIDs[i] = NO_TASK;
  }

  pthread_mutex_lock(&motion_mutex);

  // Clear the body queue
  for (std::list<const BodyJointCommand*>::iterator i = bodyQueue.begin();
       i != bodyQueue.end(); i++) {
    delete *i;
  }
  bodyQueue.clear();

  pthread_mutex_unlock(&motion_mutex);

  if (nextWalkCommand) {
    delete nextWalkCommand;
    nextWalkCommand = 0;
  }

  if (isWalkActive()) {
    cout << "Stopping the walk command" << endl;
    motionProxy->endWalk();
    walkIsStopping = true;
  }
}

const vector<float> MotionCore::getOdometry() {
  pthread_mutex_lock(&odometry_mutex);

  odometryRead = true;
  const vector <float> copy = vector<float> (latestOdometry);

  pthread_mutex_unlock(&odometry_mutex);

  return copy;
};

const float MotionCore::getHeadSpeed() {
  pthread_mutex_lock(&head_speed_mutex);

  const float copy = headSpeed;

  pthread_mutex_unlock(&head_speed_mutex);

  return copy;
}

void MotionCore::updateOdometry() {
  pthread_mutex_lock(&odometry_mutex);

  //check to see when we start at zero again
  if (odometryRead){
    latestOdometry[0] = 0.0f; //X
    latestOdometry[1] = 0.0f; //Y
    latestOdometry[2] = 0.0f; //H
  }
  odometryRead = false;

  if (!isWalkActive()){
    currentWalkVector = NOT_MOVING;
  }

  for (int i = 0; i < 3; i++){
    latestOdometry[i] += currentWalkVector[i];
  }

  pthread_mutex_unlock(&odometry_mutex);
}

void MotionCore::updateHeadSpeed() {
  pthread_mutex_lock(&head_speed_mutex);

  const float headYaw = sensors->getBodyAngle(HEAD_YAW);
  const float headPitch = sensors->getBodyAngle(HEAD_PITCH);

  // Now we can calculate the speed
  const float dY = headYaw - lastHeadYaw;
  const float dP = headPitch - lastHeadPitch;
  // we imagine that speed is a vector of two components, and total speed is
  // the length of the vector.
  headSpeed = sqrt(dY*dY + dP*dP) * static_cast<float>(MOTION_FRAME_RATE);
  
  lastHeadYaw = headYaw;
  lastHeadPitch = headPitch;
  pthread_mutex_unlock(&head_speed_mutex);
}

void MotionCore::updateSensorsWithMotion() {
  //set body angles so we can post to Sensors
  vector <float> alJointValues = motionProxy->getBodyAngles();
  vector <float> alJointErrors = motionProxy->getBodyAngleErrors();


  
  for(unsigned int i = 0; i < NUM_JOINTS; i++){
    alJointValues[i] += alJointErrors[i];
  }

  //cout << "Head joint 0 "<< alJointValues[0] << endl;
  sensors->setBodyAngles(alJointValues);
  sensors->setBodyAngleErrors(alJointErrors);
}

void
MotionCore::processCommands (void)
{
  // Decrement the times remaining to complete movement on each chain by
  // MOTION_FRAME_LENGTH.

  // KNOWN BUG!!!!
  // Sequence of steps to reproduce:
  // 1. Enqueue two joint commands.
  // 2. wait.
  // What will happen is that the first one will get enqueued and the motion
  // engine will wait until there are two motion frame lengths left before the
  // end of the first head joint command. In the loop below, we will see that
  // that's true and preemptively allow to use another postGoto call to ALmotion.
  // When we do that, we increment the chainTimeRemaining to a larger time.
  // In the for loop below though, we would like to switch over the preemptive
  // chainTaskIDs over to the normal chainTaskID in order to free up room for the
  // next preemptive chainTaskID. The odd thing is that the code seems to work
  // right now. Needs to be looked at! In any event, this is a huge improvement
  // over the previous revision even though there are known bugs in this one.

  // Boolean array that will remember which chains are currently out of work
  bool chainIsIdle[NUM_CHAINS] = { false, false, false, false, false };

  for (unsigned int i = 0; i < NUM_CHAINS; i++) {
    chainTimeRemaining[i] -= MOTION_FRAME_LENGTH_S;
    if (chainTimeRemaining[i] < 2 * MOTION_FRAME_LENGTH_S)
      chainIsIdle[i] = true;
    if (chainTimeRemaining[i] < 0) {
      chainTimeRemaining[i] = 0;
      chainTaskIDs[i] = NO_TASK;
      if (preemptiveChainTaskIDs[i] != NO_TASK) {
	chainTaskIDs[i] = preemptiveChainTaskIDs[i];
	preemptiveChainTaskIDs[i] = NO_TASK;
      }
    }
  }

  //cout << "Time remaining in headQueue: " << chainTimeRemaining[HEAD_CHAIN]
  //     << endl;

  pthread_mutex_lock(&motion_mutex);

  // If the head currently has nothing to do,
  if (chainIsIdle[HEAD_CHAIN]) {
    //cout << "The head currently has nothing to do, check for new motions in queue" << endl;
    if (headScanQueue.size() > 0 && headQueue.empty()) {
      //cout << "There is a head scan" << endl;
      const HeadScanCommand *scan = headScanQueue.front();
      const vector <const HeadJointCommand*>* commands = scan->getCommands();

      for (vector <const HeadJointCommand*>::const_iterator i = commands->begin();
	   i != commands->end(); ++i) {
	//cout << "Putting a new Joint command into headQueue" << endl;
	const HeadJointCommand *orig = *i;
	HeadJointCommand *copy = new HeadJointCommand(*orig);
	headQueue.push_back(copy);
      }

      if (!scan->isForever()) {
	//cout << "Scan should not go on forever." << endl;
	headScanQueue.pop_front();
	//cout << "Deleting the scan" << endl;
	delete scan;
      }
    }

    //cout << "headQueue.size() after scan 'if': " << headQueue.size() << endl;

    // We have this problem where pointers point to objects both in 
    // HeadJointCommand's and in HeadScanCommands, so we can't very well
    // delete them without being very careful. Think this through!!!

    // take a peek at the headQueue for new HeadJointCommands
    if (!headQueue.empty()) {
      //cout << "There is a head joint command" << endl;
      const HeadJointCommand *command = headQueue.front();
      int taskID = -1;
#ifndef NO_ACTUAL_MOTION
      //try {
      taskID = motionProxy->
	postGotoChainAngles(CHAIN_STRINGS[HEAD_CHAIN],
			    *command->getJoints(),
			    command->getDuration(),
			    command->getType());
      /*
      }catch( AL::ALError &e){
	cout <<"Failed to post head angles: \n   " << e.toString() << endl;
      }
      */
#endif
      chainTimeRemaining[HEAD_CHAIN] += command->getDuration();
      headQueue.pop_front();

      //
      if (chainTaskIDs[HEAD_CHAIN] == NO_TASK)
	chainTaskIDs[HEAD_CHAIN] = taskID;
      else
	preemptiveChainTaskIDs[HEAD_CHAIN] = taskID;

      delete command;
    }


    // Nothing to do, continue
  }

  if (!bodyQueue.empty() && !isWalkActive()) {
    //cout << "Found a body command" << endl;
    while (!bodyQueue.empty()) {
      const BodyJointCommand *command = bodyQueue.front();
      if (!command->conflicts(chainTimeRemaining)) {
	//cout<<"Command doesnt conflict"<<endl;
	for (int i = LARM_CHAIN; i <= RARM_CHAIN; i++) {
	  ChainID id = static_cast<ChainID>(i);
	  const vector <float> *chainJoints = command->getJoints(id);
	  if (chainJoints) {
	    //cout << "joints exist for chain " <<id
	    //	 << "chain pointer: " << chainJoints << endl;
	    //cout <<  "for the joints " << *chainJoints<<endl;
	    int taskID = -1;
#ifndef NO_ACTUAL_MOTION
	    // We should have try, catch here.
	    //try{
	      taskID = motionProxy->postGotoChainAngles(CHAIN_STRINGS[id],
							*chainJoints,
							command->getDuration(),
							command->getType());
	      /*
	    }catch( AL::ALError &e){
	      cout <<"Failed to post body angles: \n   " << e.toString() << endl;
	      continue;
	    }
	      */
#endif
	    chainTimeRemaining[i] += command->getDuration();

	    if (chainTaskIDs[id] == NO_TASK)
	      chainTaskIDs[id] = taskID;
	    else
	      preemptiveChainTaskIDs[id] = taskID;
	  }
	} // end for-loop
	//cout << "popping and delteing" << endl;
	bodyQueue.pop_front();
	delete command;
      }
      else break; //command conflicts, so break out of loop
    }
  }
  else if (nextWalkCommand &&
	   // and all the body chains have nothing to do
	   chainIsIdle[LLEG_CHAIN] && chainIsIdle[RLEG_CHAIN] &&
	   chainIsIdle[LARM_CHAIN] && chainIsIdle[RARM_CHAIN]) {
#ifndef NO_ACTUAL_MOTION
    const float duration = nextWalkCommand->execute(motionProxy);
    currentWalkVector = nextWalkCommand->odometry();

#endif
    delete nextWalkCommand;
    nextWalkCommand = 0;
  }
  //cout << "Size of bodyQueue: " << bodyQueue.size() << endl;
  
  pthread_mutex_unlock(&motion_mutex);
}

//
//  Thread methods
//

void
MotionCore::start (void)
{
#ifdef DEBUG_INITIALIZATION
  cout << "Motion:initializing" << endl;
  cout << "  creating threads" << endl;
#endif
  fflush(stdout);
  
  // set thread attributes
  pthread_attr_t attr;
  pthread_attr_init  (&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  // create & start thread
  pthread_create(&motion_thread, &attr, runThread, (void *)this);
  // destroy the used attributes
  pthread_attr_destroy(&attr);
#ifdef DEBUG_INITIALIZATION
  cout << "Motion:DONE" << endl;
#endif

}

void*
MotionCore::runThread (void *motion)
{
  ((MotionCore*)motion)->run();

  pthread_exit(NULL);
}


void
MotionCore::run (void)
{
  running = true;
  cout << "Running Motion" << endl;
  while (running) {
    updateSensorsWithMotion();
    processCommands();

    updateOdometry();
    updateHeadSpeed();

    usleep(static_cast<useconds_t>(MOTION_FRAME_LENGTH_uS));
  }
}

void
MotionCore::stop (void)
{
  running = false;
}


