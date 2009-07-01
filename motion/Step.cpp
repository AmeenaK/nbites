#include <string>
#include "Step.h"
#include "MotionConstants.h"
#include "Observer.h"

using namespace std;

//#define DEBUG_STEP

Step::Step(const Step& other)
{
    copyAttributesFromOther(other);
}

Step::Step(const WalkVector &target,
           const AbstractGait & gait, const Foot _foot,
	   const WalkVector &last,
	   const StepType _type)
  : walkVector(target), sOffsetY(gait.stance[WP::LEG_SEPARATION_Y]*0.5f),
    foot(_foot),type(_type),zmpd(false)
{
  copyGaitAttributes(gait.step,gait.zmp,gait.stance);

    switch(_type){
    case REGULAR_STEP:
        updateFrameLengths(stepConfig[WP::DURATION],
                           stepConfig[WP::DBL_SUPP_P]);
        break;
    case END_STEP:
        //For end steps, we are always double support, and
        //we make the length the preview period
        updateFrameLengths(static_cast<float>(Observer::NUM_PREVIEW_FRAMES) *
                           MotionConstants::MOTION_FRAME_LENGTH_S,
                           1.0f);
        break;
    }

    //After we assign elements of the gait to this step, lets clip 
    setStepSize(target,last);
}


// Copy constructor to allow changing reference frames:
Step::Step(const float new_x, const float new_y, const float new_theta,
           const Step & other)
{
    copyAttributesFromOther(other);
    x = new_x;
    y = new_y;
    theta = new_theta;
}

void Step::updateFrameLengths(const float duration,
                              const float dblSuppF){
    //need to calculate how many frames to spend in double, single
    stepDurationFrames =
        static_cast<unsigned int>( duration/
                                  MotionConstants::MOTION_FRAME_LENGTH_S);

    doubleSupportFrames =
        static_cast<unsigned int>(duration *dblSuppF/
                                  MotionConstants::MOTION_FRAME_LENGTH_S);
    singleSupportFrames = stepDurationFrames - doubleSupportFrames;
}


void Step::copyAttributesFromOther(const Step &other){
    x = other.x;
    y = other.y;
    theta = other.theta;
    walkVector = other.walkVector;
    stepDurationFrames = other.stepDurationFrames;
    doubleSupportFrames = other.doubleSupportFrames;
    singleSupportFrames = other.singleSupportFrames;
    sOffsetY= other.sOffsetY;
    foot = other.foot;
    type = other.type;
    zmpd = other.zmpd;
    copyGaitAttributes(other.stepConfig,other.zmpConfig,other.stanceConfig);
}

void Step::copyGaitAttributes(const float _step_config[WP::LEN_STEP_CONFIG],
                              const float _zmp_config[WP::LEN_ZMP_CONFIG],
			      const float _stance_config[WP::LEN_STANCE_CONFIG]){
    memcpy(stepConfig,_step_config,sizeof(float)*WP::LEN_STEP_CONFIG);
    memcpy(zmpConfig,_zmp_config,sizeof(float)*WP::LEN_ZMP_CONFIG);
    memcpy(stanceConfig,_stance_config,sizeof(float)*WP::LEN_STANCE_CONFIG);
}


void Step::setStepSize(const WalkVector &target,
		       const WalkVector &last){
  
  WalkVector new_walk =ZERO_WALKVECTOR;



#ifdef DEBUG_STEP
  printf("Input to setStepSpeed is (%g,%g,%g), (%g,%g,%g), \n",target.x,target.y,target.theta,
	 last.x,last.y,last.theta);
#endif
  //clip velocities according to the last step, only if we aren't stopping
  if(target.x != 0.0f ||  target.y != 0.0f || target.theta!= 0.0f){
    new_walk.x = NBMath::clip(target.x,
			      last.x - stepConfig[WP::MAX_ACC_X],
			      last.x + stepConfig[WP::MAX_ACC_X]);
    new_walk.y = NBMath::clip(target.y,
			      last.y - stepConfig[WP::MAX_ACC_Y],
			      last.y + stepConfig[WP::MAX_ACC_Y]);
    new_walk.theta = NBMath::clip(target.theta,
				  last.theta - stepConfig[WP::MAX_ACC_THETA],
				  last.theta + stepConfig[WP::MAX_ACC_THETA]);
  }
  //new_walk = target;
#ifdef DEBUG_STEP
  printf("After accel clipping (%g,%g,%g)\n",new_walk.x,new_walk.y,new_walk.theta);
#endif

  //clip the speeds according to the max. speeds
  new_walk.x=  NBMath::clip(new_walk.x,stepConfig[WP::MAX_VEL_X]);
  new_walk.y  = NBMath::clip(new_walk.y,stepConfig[WP::MAX_VEL_Y]);
  new_walk.theta = NBMath::clip(new_walk.theta,stepConfig[WP::MAX_VEL_THETA]);

#ifdef DEBUG_STEP
  printf("After vel clipping (%g,%g,%g)\n",new_walk.x,new_walk.y,new_walk.theta);
#endif

  //check  if we need to clip lateral movement of this leg
  if(new_walk.y > 0){
    if(!foot == LEFT_FOOT){
      new_walk.y = 0.0f;
    }
  }else if(new_walk.y < 0){
    if(foot == LEFT_FOOT){
      new_walk.y = 0.0f;
    }
  }
  
  if(new_walk.theta > 0){
    if(!foot == LEFT_FOOT){
      new_walk.theta = 0.0f;
    }
  }else if (new_walk.theta < 0){
    if(foot == LEFT_FOOT){
      new_walk.theta = 0.0f;
    }
  }

#ifdef DEBUG_STEP
  printf("After leg clipping (%g,%g,%g)\n",new_walk.x,new_walk.y,new_walk.theta);
#endif

  //Now that we have clipped the velocities, we need to convert them to distance
  //for use with this step. Note that for y and theta, we need a factor of 
  //two, since you can only stafe on every other step.

  //HACK! for bacwards compatibility, the step_y is not adjusted correctly
  const float step_x = new_walk.x*stepConfig[WP::DURATION];
  const float step_y = new_walk.y*stepConfig[WP::DURATION];//*2.0f;  
  const float step_theta = new_walk.theta*stepConfig[WP::DURATION]*2.0f;

  //Huge architectural HACK!!!  We need to fix our transforms so we don't need to do this
  //anymore
  //This hack pmakes it possible to turn about the center of the robot, rather than the 
  //center of the foot
  const float leg_sign = (foot==LEFT_FOOT ?
			  1.0f : -1.0f);
  const float computed_x = step_x - sin(std::abs(step_theta)) *sOffsetY;
  const float computed_y = step_y +
    leg_sign*sOffsetY*cos(step_theta);
  const float computed_theta = step_theta;



  x = computed_x;
  y = computed_y;
  theta = computed_theta;

  walkVector = new_walk;

#ifdef DEBUG_STEP
  std::cout << "Clipped new step to ("<<x<<","<<y<<","<<theta<<")"<<std::endl;
#endif

}
