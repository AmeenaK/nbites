/**
 * BallEKF.h - Header file for the BallEKF class
 *
 * @author Tucker Hermans
 */

#ifndef BallEKF_h_DEFINED
#define BallEKF_h_DEFINED

#include "EKF.h"
#include "MCL.h"

// Parameters
#define ASSUMED_FPS 30.0
#define BETA_BALL 5. // How much uncertainty naturally grows per update
#define GAMMA_BALL .1 // How much ball velocity should effect uncertainty
#define BALL_EKF_DIMENSION 4 // Number of states in Ball EKF
#define INIT_BALL_X 50.0f
#define INIT_BALL_Y 50.0f
#define INIT_BALL_X_VEL 0.0f
#define INIT_BALL_Y_VEL 0.0f
#define INIT_X_UNCERT 100.0f
#define INIT_Y_UNCERT 100.0f
#define INIT_X_VEL_UNCERT 100.0f
#define INIT_Y_VEL_UNCERT 100.0f

/**
 * Class for tracking of ball position and velocity.  Extends the abstract
 * EKF class.
 */
class BallEKF : public EKF
{
public:
    // Constructors & Destructors
    BallEKF(MCL _mcl, float initX, float initY,
            float initVelX, float initVelY,
            float initXUncert,float initYUncert,
            float initVelXUncert, float initVelYUncert);

    virtual ~BallEKF();

    // Getters

    /**
     * @return The current estimate of the ball x position
     */
    float getXEst() { return xhat_k(0); }

    /**
     * @return The current estimate of the ball y position
     */
    float getYEst() { return xhat_k(1); }

    /**
     * @return The current estimate of the ball x velocity
     */
    float getXVelocityEst() { return xhat_k(2); }

    /**
     * @return The current estimate of the ball y velocity
     */
    float getYVelocityEst() { return xhat_k(3); }

    /**
     * @return The current uncertainty for ball x position
     */
    float getXUncert() { return P_k(0,0); }

    /**
     * @return The current uncertainty for ball y position
     */
    float getYUncert() { return P_k(1,1); }

    /**
     * @return The current uncertainty for ball x velocity
     */
    float getXVelocityUncert() { return P_k(2,2); }

    /**
     * @return The current uncertainty for ball y velocity
     */
    float getYVelocityUncert() { return P_k(3,3); }

private:
    // Core Functions
    virtual ublas::vector<float> associateTimeUpdate(MotionModel u_k);
    virtual ublas::vector<float> incorporateMeasurement(Measurement z,
                                                        ublas::
                                                        matrix<float> &H_k,
                                                        ublas::
                                                        matrix<float> &R_k);
    // Members
    MCL robotLoc;
};
#endif // File
