
#include "MotionInterface.h"

void MotionInterface::setNextWalkCommand(const WalkCommand *command){
    cout << "Walk command in the interface" <<endl;
    switchboard->sendMotionCommand(reinterpret_cast<const MotionCommand *> (command));
}

void MotionInterface::enqueue(const BodyJointCommand *command){
    cout << "Interface got a BodyJointCommand" <<endl;
    switchboard->sendMotionCommand(reinterpret_cast<const MotionCommand *> (command));
}

void MotionInterface::enqueue(const HeadJointCommand *command){
    cout << "Interface got a HeadJointCommand" <<endl;
    switchboard->sendMotionCommand(reinterpret_cast<const MotionCommand *> (command));
}

void MotionInterface::enqueue(const HeadScanCommand *command){
    switchboard->sendMotionCommand(reinterpret_cast<const MotionCommand *> (command));
}

void MotionInterface::stopBodyMoves() {
}

void MotionInterface::stopHeadMoves() {
}

float MotionInterface::getHeadSpeed() {
    return DUMMY_F;
}

void MotionInterface::setWalkConfig ( float pMaxStepLength, float pMaxStepHeight,
				      float pMaxStepSide, float pMaxStepTurn,
				      float pZmpOffsetX, float pZmpOffsetY) {
}

void MotionInterface::setWalkArmsConfig ( float pShoulderMedian,
					  float pShoulderApmlitude,
					  float pElbowMedian,
					  float pElbowAmplitude) {
}

void MotionInterface::setWalkExtraConfig( float pLHipRollBacklashCompensator,
					  float pRHipRollBacklashCompensator,
					  float pHipHeight,
					  float pTorsoYOrientation) {
}

void MotionInterface::setWalkParameters( const WalkParameters& params) {
}


void MotionInterface::setSupportMode(int pSupportMode) {
}

int MotionInterface::getSupportMode() {
    return DUMMY_I;
}


void MotionInterface::setBalanceMode(int pBalanceMode) {
}

int MotionInterface::getBalanceMode() {
    return DUMMY_I;
}
