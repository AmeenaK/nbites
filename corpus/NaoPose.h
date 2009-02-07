
// This file is part of Man, a robotic perception, locomotion, and
// team strategy application created by the Northern Bites RoboCup
// team of Bowdoin College in Brunswick, Maine, for the Aldebaran
// Nao robot.
//
// Man is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Man is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser Public License for more details.
//
// You should have received a copy of the GNU General Public License
// and the GNU Lesser Public License along with Man.  If not, see
// <http://www.gnu.org/licenses/>.


/**
 * NaoPose: The class that is responsible for calculating the pose of the robot
 * in order that the vision system can tell where the objects it sees are located
 * with respect to the center of the body.
 *
 * To understand this file, please review the modified Denavit Hartenberg
 * convention on robotics coordinate frame transformations, as well as the
 * following documentation:
 *  * Nao Camera documentation
 *  * NaoWare documentation
 *  * HONG NUBots Enhancements to vision 2005 (for Horizon)
 *  * Wiki line-plane intersection:  en.wikipedia.org/wiki/Line-plane_intersection
 *
 * The following coordinate frames will be important:
 *  * The camera frame. origin at the focal point, alligned with the head.
 *  * The body frame. origin at the center of mass (com), alligned with the torso.
 *  * The world frame. orgin at the center of mass, alligned with the ground
 *  * The horizon frame. origin at the focal point, alligned with the ground
 *
 * The following methods are central to the functioning of NaoPose:
 *  * tranform()     - must be called each frame. setups transformation matrices
 *                     for the legs and the camera, which enables calculation of
 *                     body height and horizon
 *
 *  * pixEstimate()  - returns an estimate to a given x,y pixel, representing an
 *                     object at a certain height from the ground. Takes untis of
 *                     CM, and returns in CM. See also bodyEstimate
 *
 *  * bodyEstimate() - returns an estimate for a given x,y pixel, and a distance
 *                     calculated from vision by blob size. See also pixEstimate
 *
 *
 *  * Performance Profile: mDH for 3 chains takes 70% time in tranform(), and
 *                         horizon caculation takes the other 30%. pixEstimate()
 *                         and bodyEstimate() take two orders of magnitude less
 *                         time than transform()
 *  * Optimization: Eventually we'll calculate the world frame rotation using
 *                  gyros, so we won't need to use the support leg's orientation
 *                  in space to figure this out. That will save quite a bit of
 *                  computation.
 *
 * @date July 2008
 * @author George Slavov
 * @author Johannes Strom
 *
 * TODO:
 *   - Need to fix the world frame, since it currently relies on the rotation of
 *     the foot also about the Z axis, which means when we are turning, for
 *     example, we will report distances and more imporantly bearings relative
 *     to the foot instead of the body. This has been hacked for now, in 
 *     bodyEstimate.
 *
 *   - While testing this code on the robot, we saw a significant, though
 *     infrequent, jump in the estimate for the horizon line. The cause could be
 *     one of bad joint information from the motion engine, a singular matrix
 *     inversion we do somewhere in the code, or something completely different.
 *     Needs to be looked at.
 *
 *   - While testing this code on the robot, we have noticed severe
 *     discrepancies between distances reported from pixEstimate, and their
 *     actual values. While the model seems to work well in general, there
 *     needs to be a serious evaluation of these defects, since these
 *     inaccuracies will have serious consequences for localization.
 *
 */

#ifndef _NaoPose_h_DEFINED
#define _NaoPose_h_DEFINED

#include <cmath>
#include <vector>
#include <algorithm>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/triangular.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/fwd.hpp>             // for matrix_column
#include <boost/numeric/ublas/lu.hpp>              // for lu_factorize
#include <boost/numeric/ublas/io.hpp>              // for cout
using namespace boost::numeric;

#include "Common.h"             // For ROUND
#include "VisionDef.h"          // For camera parameters
#include "Kinematics.h"         // For physical body parameters

using namespace Kinematics;

#include "Sensors.h"
#include "Structs.h"

/**
 *  *****************   Various constants follow   **********************
 */

using namespace std;

// CONSTANTS

struct mDHParam{
  float xRot; //xRot
  float xTrans; //xTrans
  float zRot; //zRot
  float zTrans; //zTrans

};

// Various Body Constants ALL IN MILLIMETERS

static const float NECK_LENGTH = 80.0f;
static const float CAMERA_YAW_Y_LENGTH = 81.06f;
static const float CAMERA_YAW_Z_LENGTH = 14.6f;
static const float NECK_BASE_TO_CENTER_Y = 67.5f;
static const float NECK_BASE_TO_CENTER_Z = 19.5f;
static const float SHOULDER_TO_ELBOW_LENGTH = 69.5f;
static const float ELBOW_TO_ELBOW_X = 4.7f;
static const float ELBOW_TO_ELBOW_Y = -9.0f;
static const float REAR_LEG_LENGTH = 76.5f;// 79.4;--UTexas value
static const float REAR_LEG_TO_CENTER_BODY_Y = 65.0f;
static const float REAR_LEG_TO_CENTER_BODY_X = 62.5f;
static const float NECK_TILT2_TO_CAMERA_Y = 81.0f;
static const float NECK_TILT2_TO_CAMERA_Z = -14.6f;
static const float NECK_TILT_TO_TILT2 = 80.0f;
static const float SHOULDER_TO_NECK_TILT_Y = 2.5f;
static const float SHOULDER_TO_NECK_TILT_Z = 19.5f;
// Horizon-Related Constants
static const float MM_TO_PIX_X_PLANE = 0.0171014f;
static const float MM_TO_PIX_Y_PLANE = 0.0171195f;
static const float HORIZON_LEFT_X_MM = -1.76999f;
static const float HORIZON_RIGHT_X_MM = 1.76999f;
static const float FOCAL_LENGTH_PIX = 192.0f;
static const float FOCAL_LENGTH_MM = 3.27f;
static const float IMAGE_CENTER_X = 103.5f;
static const float IMAGE_CENTER_Y = 79.5f;
static const float HALF_FOV_X_MM = 1.77f;
static const float HALF_FOV_Y_MM = 1.361f;
static const float IMAGE_MM_WIDTH = 2.0f*tan(FOV_X/2.0f)*FOCAL_LENGTH_MM;
static const float IMAGE_MM_HEIGHT = 2.0f*tan(FOV_Y/2.0f)*FOCAL_LENGTH_MM;
// e.g. 3 mm * mm_to_pix = 176 pixels
static const float MM_TO_PIX_X = IMAGE_WIDTH/IMAGE_MM_WIDTH;
static const float MM_TO_PIX_Y = IMAGE_HEIGHT/IMAGE_MM_HEIGHT;

// 3d origin constant
static const point3 <float> ZERO_COORD(0.0f,0.0f,0.0f);
// a null estimate struct. zero dist,bearing,elevation (check dist always), x, y
static const estimate NULL_ESTIMATE = {0.0f,0.0f,0.0f,0.0f,0.0f};
// defines the left and right horizontal points in mm and in xyz space
static const point3 <float> HORIZON_LEFT_3D ( HORIZON_LEFT_X_MM, FOCAL_LENGTH_MM, 0.0);
static const point3 <float> HORIZON_RIGHT_3D (HORIZON_RIGHT_X_MM, FOCAL_LENGTH_MM, 0.0 );
static const float LINE_HEIGHT = 0.0f;


class NaoPose {
 protected: // Constants
  static const float IMAGE_WIDTH_MM;
  static const float IMAGE_HEIGHT_MM;
  static const float FOCAL_LENGTH_MM;
  static const float MM_TO_PIX_X;
  static const float MM_TO_PIX_Y;
  static const float PIX_X_TO_MM;
  static const float PIX_Y_TO_MM;
  static const float IMAGE_CENTER_X;
  static const float IMAGE_CENTER_Y;

  static const float PIX_TO_DEG_X;
  static const float PIX_TO_DEG_Y;

  static const estimate NULL_ESTIMATE;

  static const float INFTY;
  // Screen edge coordinates in the camera coordinate frame.
  static const ublas::vector <float> topLeft, bottomLeft, topRight, bottomRight;

  // We use this to access the x,y,z components of vectors
  enum Cardinal {
    X = 0,
    Y = 1,
    Z = 2
  };

 public:
  NaoPose(boost::shared_ptr<Sensors> s);
  ~NaoPose() { }

  /********** Core Methods **********/
  void transform ();

  const estimate pixEstimate(const int pixelX, const int pixelY,
				     const float objectHeight);
  const estimate bodyEstimate(const int x, const int y, const float dist);

  /********** Getters **********/
  const int getHorizonY(const int x) const;
  const int getHorizonX(const int y) const;
  const point <int> getLeftHorizon() const { return horizonLeft; }
  const point <int> getRightHorizon() const { return horizonRight; }
  const int getLeftHorizonY() const { return horizonLeft.y; }
  const int getRightHorizonY() const { return horizonRight.y; }
  const float getHorizonSlope() const { return horizonSlope; }
  const float getPerpenSlope() const { return perpenHorizonSlope; }

 protected: // helper methods
  static const ublas::matrix <float>
    calculateForwardTransform(const ChainID id,
			      const std::vector <float> &angles);
  static const ublas::vector <float> calcFocalPointInBodyFrame();

  void calcImageHorizonLine();
  // This method solves a system of linear equations and return a 3-d vector
  // in homogeneous coordinates representing the point of intersection
  static ublas::vector <float>
    intersectLineWithXYPlane(const std::vector<ublas::vector <float> > &aLine);
  // In homogeneous coordinates, get the length of a n-dimensional vector.
  static const float getHomLength(const ublas::vector <float> &vec);
  // takes in two sides of a triangle, returns hypotenuse
  
  static const double getHypotenuse(const float x, const float y) {
    return sqrt(pow(x,2)+pow(y,2));
  }

  //returns an 'estimate' object for a homogeneous vector pointing to an
  //object in the world frame
  static estimate getEstimate(ublas::vector <float> objInWorldFrame);

 protected: // members
  boost::shared_ptr<Sensors> sensors;
  point <int> horizonLeft, horizonRight;
  float horizonSlope,perpenHorizonSlope;;
  point3 <float> focalPointInWorldFrame;
  float comHeight; // center of mass height in mm
  // In this array we hold the matrix transformation which takes us from the
  // originn to the end of each chain of the body.
  ublas::matrix <float> forwardTransforms[NUM_CHAINS];
  // World frame is defined as the coordinate frame centered at the center of
  // mass and parallel to the ground plane.
  ublas::matrix <float> cameraToWorldFrame;
  // Current hack for better beraing est
  ublas::matrix <float> cameraToBodyTransform;
};

#endif
