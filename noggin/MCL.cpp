/**
 * MCL.cpp
 *
 * File houses the main parts of the Monte Carlo Localization system
 *
 * @author Tucker Hermans
 */

#include "MCL.h"
using namespace std;

/**
 * Initializes the sampel sets so that the first update works appropriately
 */
MCL::MCL()
{
    // Initialize particles to be randomly spread about the field...
    srand(time(NULL));
    for (int m = 0; m < M; ++m) {
        //Particle p_m;
        // X bounded by width of the field
        // Y bounded by height of the field
        // H between +-pi
        PoseEst x_m(float(rand() % int(FIELD_WIDTH)),
                    float(rand() % int(FIELD_HEIGHT)),
                    float(((rand() % FULL_CIRC) - HALF_CIRC))*DEG_TO_RAD);
        Particle p_m(x_m, 1.0f);
        X_t.push_back(p_m);
    }

    updateEstimates();
}

MCL::~MCL()
{
}

/**
 * Method updates the set of particles and estimates the robots position.
 * Called every frame.
 *
 * @param u_t The motion (odometery) change since the last update.
 * @param z_t The set of landmark observations in the current frame.
 * @param resample Should we resample during this update
 * @return The set of particles representing the estimate of the current frame.
 */
void MCL::updateLocalization(MotionModel u_t, vector<Observation> z_t,
                             bool resample)
{
    // Set the current particles to be of time minus one.
    vector<Particle> X_t_1 = X_t;
    // Clar the current set
    X_t.clear();
    vector<Particle> X_bar_t; // A priori estimates
    float totalWeights = 0.; // Must sum all weights for future use

    // Run through the particles
    for (int m = 0; m < M; ++m) {
        Particle x_t_m;
        // Update motion model for the particle
        x_t_m.pose = updateOdometery(u_t, X_t_1[m].pose);

        // Update measurement model
        x_t_m.weight = updateMeasurementModel(z_t, x_t_m.pose);
        totalWeights += x_t_m.weight;
        // Add the particle to the current frame set
        X_bar_t.push_back(x_t_m);
    }

    // Process the particles after updating them all
    for (int m = 0; m < M; ++m) {

        // Normalize the particle weights
        X_bar_t[m].weight /= totalWeights;

        if(resample) { // Resample the particles
            int count = int(round(float(M) * X_bar_t[m].weight));
            // Add the particles to the resample posterior!
            for (int i = 0; i < count; ++i) {
                // Random walk the particles
                X_t.push_back(randomWalkParticle(X_bar_t[m]));
            }

        } else { // Keep particle count the same
            // Random walk the particles
            X_t.push_back(randomWalkParticle(X_bar_t[m]));
        }

    }

    // Update pose and uncertainty estimates
    updateEstimates();
}

/**
 * Method updates the robot pose based on motion changes since the last frame.
 *
 * @param u_t The motion change since the last update.
 * @param x_t The robot pose estimate to be updated.
 * @return The new estimated robot pose.
 */
PoseEst MCL::updateOdometery(MotionModel u_t, PoseEst x_t)
{
    // Translate the relative change into the global coordinate system
    // float deltaX, deltaY, deltaH;
    // float calcFromAngle = x_t.h + M_PI / 2.0f;
    // deltaX = u_t.deltaF * cos(calcFromAngle) - u_t.deltaL * sin(calcFromAngle);
    // deltaY = u_t.deltaF * sin(calcFromAngle) - u_t.deltaL * cos(calcFromAngle);
    // deltaH = u_t.deltaR; // Rotational change is the same as heading change

    // // Add the change to the current pose estimate
    // x_t.x += deltaX;
    // x_t.y += deltaY;
    // x_t.h += deltaH;

    x_t += u_t;
    return x_t;
}

/**
 * Method determines the weight of a particle based on the current landmark
 * observations.
 *
 * @param z_t The landmark observations for the current frame.
 * @param x_t The a priori estimate of the robot pose.
 * @param m The particle ID
 * @return The particle weight
 */
float MCL::updateMeasurementModel(vector<Observation> z_t, PoseEst x_t)
{
    // Give the particle a weight of 1 to begin with
    float w = 1;

    //cout << "New Particle" << endl;
    // Determine the likelihood of each observation
    for (unsigned int i = 0; i < z_t.size(); ++i) {

        // Determine the most likely match
        float p = 0; // Combined probability of observation
        float pMax = -1; // Maximum combined probability

        // Get local copies of our possible landmarks
        vector<LineLandmark> possibleLines = z_t[i].getLinePossibilities();
        vector<PointLandmark> possiblePoints = z_t[i].getPointPossibilities();

        // Loop through all possible landmarks
        // If the observation is distinct, there will only be one possibility
        for (unsigned int j = 0; j < z_t[i].getNumPossibilities(); ++j) {
            if (z_t[i].isLine()) {
                p = determineLineWeight(z_t[i], x_t, possibleLines[j]);
            } else {
                p = determinePointWeight(z_t[i], x_t, possiblePoints[j]);
            }

            //cout << "p is: " << p << " ";
            if( p > pMax) {
                pMax = p;
            }
        }
        //cout << "pMax is: " << pMax << endl;
        w *= pMax;
    }

    return w; // The combined weight of all observations
}

/**
 * Method to update the robot pose and uncertainty estimates.
 * Calculates the weighted mean and biased standard deviations of the particles.
 */
void MCL::updateEstimates()
{
    PoseEst wMeans(0.,0.,0.);
    float weightSum = 0.;
    PoseEst bSDs(0., 0., 0.);

    // Calculate the weighted mean
    for (unsigned int i = 0; i < X_t.size(); ++i) {
        // Sum the values
        wMeans.x += X_t[i].pose.x*X_t[i].weight;
        wMeans.y += X_t[i].pose.y*X_t[i].weight;
        wMeans.h += X_t[i].pose.h*X_t[i].weight;

        // Sum the weights
        weightSum += X_t[i].weight;
    }

    wMeans.x /= weightSum;
    wMeans.y /= weightSum;
    wMeans.h /= weightSum;

    // Calculate the biased variances
    for (unsigned int i=0; i < X_t.size(); ++i) {
        bSDs.x += X_t[i].weight * (X_t[i].pose.x - wMeans.x)*
            (X_t[i].pose.x - wMeans.x);
        bSDs.y += X_t[i].weight * (X_t[i].pose.y - wMeans.y)*
            (X_t[i].pose.y - wMeans.y);
        bSDs.h += X_t[i].weight * (X_t[i].pose.h - wMeans.h)*
            (X_t[i].pose.h - wMeans.h);
    }

    bSDs.x /= weightSum;
    bSDs.x = sqrt(bSDs.x);

    bSDs.y /= weightSum;
    bSDs.y = sqrt(bSDs.y);

    bSDs.h /= weightSum;
    bSDs.h = sqrt(bSDs.h);

    // Set class variables to reflect newly calculated values
    curEst = wMeans;
    curUncert = bSDs;
}

//Helpers

/**
 * Determine the weight to compound the particle weight by for a single
 * observation of an landmark point
 *
 * @param z    the observation to determine the weight of
 * @param x_t  the a priori estimate of the robot pose.
 * @param l    the landmark to be used as basis for the observation
 * @return     the probability of the observation
 */
float MCL::determinePointWeight(Observation z, PoseEst x_t, PointLandmark pt)
{
    // Expected dist and bearing
    float d_hat;
    float a_hat;
    // Residuals of distance and bearing observations
    float r_d;
    float r_a;

    // Determine expected distance to the landmark
    d_hat = sqrt( (pt.x - x_t.x)*(pt.x - x_t.x) +
                  (pt.y - x_t.y)*(pt.y - x_t.y));
    // Expected bearing
    a_hat = atan2(pt.y - x_t.y, pt.x - x_t.x) + x_t.h - QUART_CIRC_RAD;

    // Calculate residuals
    r_d = z.getVisDistance() - d_hat;
    r_a = z.getVisBearing() - a_hat;

    return getSimilarity(r_d, r_a, z);
}

/**
 * Determine the simalirty between the observed and expected of a line.
 *
 * @param z    the observation to determine the weight of
 * @param x_t  the a priori estimate of the robot pose.
 * @param l    the landmark to be used as basis for the observation
 * @return     the probability of the observation
 */
float MCL::determineLineWeight(Observation z, PoseEst x_t, LineLandmark line)
{
    // Distance and bearing for expected point
    float d_hat;
    float a_hat;

    // Residuals of distance and bearing observations
    float r_d;
    float r_a;

    // Determine nearest expected point on the line
    // NOTE:  This is currently too naive, we need to extrapolate the entire
    // line from the vision inforamtion and find the closest expected distance
    PointLandmark pt_hat;
    if (line.x1 == line.x2) { // Line is vertical
        pt_hat.x = line.x1;
        pt_hat.y = x_t.y;

        // Make sure we aren't outside the line
        if ( pt_hat.y < line.y1) {
            pt_hat.y = line.y1;
        } else if ( pt_hat.y > line.y2) {
            pt_hat.y = line.y2;
        }
    } else { // Line is horizontal
        pt_hat.x = x_t.x;
        pt_hat.y = line.y1;

        // Make sure we aren't outside the line
        if ( pt_hat.x < line.x1) {
            pt_hat.x = line.x1;
        } else if ( pt_hat.x > line.x2) {
            pt_hat.x = line.x2;
        }
    }

    // Get dist and bearing to expected point
    d_hat = sqrt( pow(pt_hat.x - x_t.x, 2.0f) +
                  pow(pt_hat.y - x_t.y, 2.0f));

    // Expected bearing
    a_hat = atan2(pt_hat.y - x_t.y, pt_hat.x - x_t.x) - x_t.h;

    // Calculate residuals
    r_d = fabs(z.getVisDistance() - d_hat);
    r_a = fabs(z.getVisBearing() - a_hat);

    return getSimilarity(r_d, r_a, z);
}

/**
 * Determine the similarity of an observation to a landmark location
 *
 * @param r_d     The difference between the expected and observed distance
 * @param r_a     The difference between the expected and observed bearing
 * @param sigma_d The standard deviation of the distance measurement
 * @param sigma_a The standard deviation of the bearing measurement
 * @return        The combined similarity of the landmark observation
 */
float MCL::getSimilarity(float r_d, float r_a, Observation &z)
{
    // Similarity of observation and expectation
    float s_d_a;
    float sigma_d = z.getDistanceSD();
    float sigma_a = z.getBearingSD();
    // Calculate the similarity of the observation and expectation
    s_d_a = exp((-(r_d*r_d) / (sigma_d*sigma_d))
                -((r_a*r_a) / (sigma_a*sigma_a)));

    if (s_d_a < MIN_SIMILARITY) {
        s_d_a = MIN_SIMILARITY;
    }
    return s_d_a;
}

Particle MCL::randomWalkParticle(Particle p)
{
    p.pose.x += MAX_CHANGE_X * (1.0f - p.weight)*UNIFORM_1_NEG_1;
    p.pose.y += MAX_CHANGE_Y * (1.0f - p.weight)*UNIFORM_1_NEG_1;
    p.pose.h += MAX_CHANGE_H * (1.0f - p.weight)*UNIFORM_1_NEG_1;
    return p;
}

// Other class simple constructors
// PoseEst
PoseEst::PoseEst(float _x, float _y, float _h) :
    x(_x), y(_y), h(_h)
{
}

PoseEst::PoseEst(const PoseEst& other) :
    x(other.x), y(other.y), h(other.h)
{
}

PoseEst::PoseEst() {}

// MotionModel
MotionModel::MotionModel(float f, float l, float r) :
    deltaF(f), deltaL(l), deltaR(r)
{
}

MotionModel::MotionModel(const MotionModel& other) :
    deltaF(other.deltaF), deltaL(other.deltaL), deltaR(other.deltaR)
{
}

MotionModel::MotionModel(){}

// Particle
Particle::Particle(PoseEst _pose, float _weight) :
    pose(_pose), weight(_weight)
{
}

Particle::Particle(const Particle& other) :
    pose(other.pose), weight(other.weight)
{
}

Particle::Particle(){}
