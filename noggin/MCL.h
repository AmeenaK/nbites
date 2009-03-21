/**
 * MCL.h
 *
 * @author Tucker Hermans
 * @version %I%
 */

#ifndef MCL_h_DEFINED
#define MCL_h_DEFINED

// Includes
// STL
#include <vector>
#include <math.h>
#include <time.h> // for srand(time(NULL))
#include <cstdlib> // for MAX_RAND
// Local
#include "Observation.h"
#include "FieldConstants.h"
#include "NBMath.h"
// Structs
// Odometery change
class MotionModel
{
public:
    MotionModel(float f, float l, float r);
    MotionModel(const MotionModel& other);
    MotionModel();
    float deltaF;
    float deltaL;
    float deltaR;
};

// Pose Estimate
class PoseEst
{
public:
    // Constructors
    PoseEst(float _x, float _y, float _h);
    PoseEst();
    PoseEst(const PoseEst& other);
    float x;
    float y;
    float h;

    PoseEst operator+ (const PoseEst o)
    {
        return PoseEst(o.x + x,
                       o.y + y,
                       o.h + h);
    }
    void operator+= (const PoseEst o)
    {
        x += o.x;
        y += o.y;
        h += o.h;
    }
    PoseEst operator+ (const MotionModel u_t)
    {
        // Translate the relative change into the global coordinate system
        // And add that to the current estimate
        float calcFromAngle = h - M_PI / 2.0f;
        return PoseEst(u_t.deltaF * -cos(calcFromAngle) +
                       u_t.deltaL * sin(calcFromAngle),
                       u_t.deltaF * -sin(calcFromAngle) -
                       u_t.deltaL * cos(calcFromAngle),
                       h += u_t.deltaR);
    }
    void operator+= (const MotionModel u_t)
    {
        // Translate the relative change into the global coordinate system
        // And add that to the current estimate
        float calcFromAngle = h - M_PI / 2.0f;
        x += u_t.deltaF * -cos(calcFromAngle) + u_t.deltaL * sin(calcFromAngle);
        y += u_t.deltaF * -sin(calcFromAngle) - u_t.deltaL * cos(calcFromAngle);
        h += u_t.deltaR;
    }

  friend std::ostream& operator<< (std::ostream &o, const PoseEst &c)
  {
      return o << "(" << c.x << ", " << c.y << ", " << c.h << ")";
  }


};

// Particle
class Particle
{
public:
    Particle(PoseEst _pose, float _weight);
    Particle(const Particle& other);
    Particle();
    PoseEst pose;
    float weight;
    //BallEKF ball;
    //std::vector<EKF> opponents;

    friend std::ostream& operator<< (std::ostream &o, const Particle &c) {
        return o << c.pose.x << " " << c.pose.y << " " << c.pose.h << " "
                 << c.weight;
    }

};

// Constants
static const int M = 100; // Number of particles
static const float MIN_SIMILARITY = 1.0e-15; // Minimum possible similarity

// The Monte Carlo Localization class
class MCL
{
public:
    // Constructors & Destructors
    MCL();
    virtual ~MCL();

    // Core Functions
    void updateLocalization(MotionModel u_t, std::vector<Observation> z_t,
                            bool resample=true);

    // Getters
    PoseEst getCurrentEstimate() { return curEst; }
    PoseEst getCurrentUncertainty() { return curUncert; }
    /**
     * @return The current x esitamte of the robot
     */
    float getXEst() { return curEst.x;}

    /**
     * @return The current y esitamte of the robot
     */
    float getYEst() { return curEst.y;}

    /**
     * @return The current heading esitamte of the robot in radians
     */
    float getHEst() { return curEst.h;}

    /**
     * @return The current heading esitamte of the robot in degrees
     */
    float getHEstDeg() { return curEst.h * DEG_OVER_RAD;}

    /**
     * @return The uncertainty associated with the x estimate of the robot.
     */
    float getXUncert() { return curUncert.x * 2;}

    /**
     * @return The uncertainty associated with the y estimate of the robot.
     */
    float getYUncert() { return curUncert.y * 2;}

    /**
     * @return The uncertainty associated with the robot's heading estimate.
     */
    float getHUncert() { return curUncert.h * 2;}

    /**
     * @return The uncertainty associated with the robot's heading estimate.
     */
    float getHUncertDeg() { return curUncert.h * 2 * DEG_OVER_RAD;}

    /**
     * @return The current set of particles in the filter
     */
    std::vector<Particle> getParticles() { return X_t; }

    // Setters
    /**
     * @param xEst The current x esitamte of the robot
     */
    void setXEst(float xEst) { curEst.x = xEst;}

    /**
     * @param yEst The current y esitamte of the robot
     */
    void setYEst(float yEst) { curEst.y = yEst;}

    /**
     * @param hEst The current heading esitamte of the robot
     */
    void setHEst(float hEst) { curEst.h = hEst;}

    /**
     * @param uncertX The uncertainty of the x estimate of the robot.
     */
    void setXUncert(float uncertX) { curUncert.x = uncertX;}

    /**
     * @return uncertY The uncertainty of the y estimate of the robot.
     */
    void setYUncert(float uncertY) { curUncert.y = uncertY;}

    /**
     * @param uncertH The uncertainty of the robot's heading estimate.
     */
    void setHUncert(float uncertH) { curUncert.h = uncertH;}

private:
    // Class variables
    PoseEst curEst; // Current {x,y,h} esitamates
    PoseEst curUncert; // Associated {x,y,h} uncertainties (standard deviations)
    std::vector<Particle> X_t; // Current set of particles

    // Core Functions
    float updateMeasurementModel(std::vector<Observation> z_t, PoseEst x_t);
    void updateEstimates();

    // Helpers
    float determinePointWeight(Observation z, PoseEst x_t,
                               PointLandmark landmark);
    float determineLineWeight(Observation z, PoseEst x_t, LineLandmark _line);
    float getSimilarity(float r_d, float r_a, Observation &z);
    Particle randomWalkParticle(Particle p);

public:
    friend std::ostream& operator<< (std::ostream &o, const MCL &c) {
        return o << "Est: " << c.curEst << "\nUnct: " << c.curUncert;
    }

};

#endif // _MCL_H_DEFINED
