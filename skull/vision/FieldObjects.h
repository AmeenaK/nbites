#ifndef FieldObjects_h_DEFINED
#define FieldObjects_h_DEFINED
using namespace std;

#include "Common.h"
#include "Structs.h"
#include "ifdefs.h"
#include "FieldConstants.h"

////////////////////////////////////////////////////////////
// Constants for absolute coordinates on the field of the six
// most meaningful field objects (in terms of stationary things
// we extract distance information from).  Since we do not use
// backstops for any distance/bearing information, I am not including
// it here.
// (0,0) is the lower left corner of the field when BLUE goal is at the
// bottom
// NICK
//
////////////////////////////////////////////////////////////

static const point <double> BLUE_GOAL_LEFT_POST_LOC =
  point<double>(LANDMARK_MY_GOAL_LEFT_POST_X,
                LANDMARK_MY_GOAL_LEFT_POST_Y);

static const point <double> BLUE_GOAL_RIGHT_POST_LOC =
  point<double>(LANDMARK_MY_GOAL_RIGHT_POST_X,
                LANDMARK_MY_GOAL_RIGHT_POST_Y);

static const point <double> YELLOW_GOAL_LEFT_POST_LOC =
  point<double>(LANDMARK_OPP_GOAL_RIGHT_POST_X,
                LANDMARK_OPP_GOAL_RIGHT_POST_Y);

static const point <double> YELLOW_GOAL_RIGHT_POST_LOC =
  point<double>(LANDMARK_OPP_GOAL_LEFT_POST_X,
                LANDMARK_OPP_GOAL_LEFT_POST_Y);

static const point <double> YELLOW_BLUE_BEACON_LOC =
  point<double>(LANDMARK_LEFT_BEACON_X, LANDMARK_LEFT_BEACON_Y);

static const point <double> BLUE_YELLOW_BEACON_LOC =
  point<double>(LANDMARK_RIGHT_BEACON_X, LANDMARK_RIGHT_BEACON_Y);

// Arc locations are defined to be the center of the quarter circle spanned
// by the arc
static const point <double> BLUE_GOAL_YELLOW_ARC_CENTER_LOC = 
  point<double>(FIELD_WHITE_LEFT_SIDELINE_X, FIELD_WHITE_BOTTOM_SIDELINE_Y);

static const point <double> BLUE_GOAL_BLUE_ARC_CENTER_LOC = 
  point<double>(FIELD_WHITE_RIGHT_SIDELINE_X, FIELD_WHITE_BOTTOM_SIDELINE_Y);

static const point <double> YELLOW_GOAL_YELLOW_ARC_CENTER_LOC = 
  point<double>(FIELD_WHITE_RIGHT_SIDELINE_X, FIELD_WHITE_TOP_SIDELINE_Y);

static const point <double> YELLOW_GOAL_BLUE_ARC_CENTER_LOC = 
  point<double>(FIELD_WHITE_LEFT_SIDELINE_X, FIELD_WHITE_TOP_SIDELINE_Y);






static const point <double> UNKNOWN_LOC =
  point<double>(-1, -1);

enum fieldObjectID {
  BLUE_GOAL_LEFT_POST,
  BLUE_GOAL_RIGHT_POST,
  YELLOW_GOAL_LEFT_POST,
  YELLOW_GOAL_RIGHT_POST,
  YELLOW_BLUE_BEACON,
  BLUE_YELLOW_BEACON,
  BLUE_ARC,
  YELLOW_ARC,
  UNKNOWN_FIELD_OBJECT
};

class FieldObjects;
#include "Vision.h"

class FieldObjects {
 public:
  FieldObjects(Vision *vis, const fieldObjectID);
  FieldObjects(Vision *vis);
  FieldObjects();
  virtual ~FieldObjects() {}

  friend std::ostream& operator<< (std::ostream &o, const FieldObjects &l)
  {
    return o << l.toString() << "\tWidth: " << l.width
             << "\tHeight: " << l.height
             << "\tDistance: " << l.dist 
             << "\tAngle X: " << l.angleX << "\tAngle Y: " << l.angleY;
  }

  // INITIALIZATION (happens every frame)
  void init();

  void printDebugInfo(FILE * out);
  static string getStringFromID(fieldObjectID _id);

  // SETTERS
  void setWidth(double w) { width = w; }
  void setHeight(double h) { height = h; }
  void setX(int x1) { x = x1; }
  void setY(int y1) { y = y1; }
  void setLeftTopX(int _x){  leftTop.x = _x; }
  void setLeftTopY(int _y){  leftTop.y = _y; }
  void setRightTopX(int _x){ rightTop.x = _x; }
  void setRightTopY(int _y){ rightTop.y = _y; }
  void setLeftBottomX(int _x){ leftBottom.x = _x; }
  void setLeftBottomY(int _y){ leftBottom.y = _y; }
  void setRightBottomX(int _x){ rightBottom.x = _x; }
  void setRightBottomY(int _y){ rightBottom.y = _y; }
  void setCenterX(int cX) { centerX = cX; }
  void setCenterY(int cY) { centerY = cY; }
  void setAngleX(double aX) { angleX = aX; }
  void setAngleY(double aY) { angleY = aY; }
  void setBearing(double b) { bearing = b; }
  void setElevation(double e) { elevation = e; }
  void setFocDist(double fd) { focDist = fd; }
  void setDist(double d) { dist = d; }
  void setCertainty(int c) {certainty = c; }
  void setDistCertainty(int c) {distCertainty = c;}
  void setShoot(bool s1) {shoot = s1;}
  void setBackLeft(int x1) {backLeft = x1;}
  void setBackRight(int y1) {backRight = y1;}
  void setBackDir(int x1) {backDir = x1;}
  void setLeftOpening(int op) { leftOpening = op; }
  void setRightOpening(int op) { rightOpening = op; }

  // GETTERS
  double getWidth() const { return width; }
  double getHeight() const { return height; }
  int getX() const{ return x; }
  int getY() const{ return y; }
  int getLeftTopX() const{ return leftTop.x; }
  int getLeftTopY() const{ return leftTop.y; }
  int getRightTopX() const{ return rightTop.x; }
  int getRightTopY() const{ return rightTop.y; }
  int getLeftBottomX() const{ return leftBottom.x; }
  int getLeftBottomY() const{ return leftBottom.y; }
  int getRightBottomX() const{ return rightBottom.x; }
  int getRightBottomY() const{ return rightBottom.y; }
  int getCenterX() const { return centerX; }
  int getCenterY() const { return centerY; }
  int getCertainty() const { return certainty; }
  int getDistCertainty() const { return distCertainty; }
  double getAngleX() const { return angleX; }
  double getAngleY() const { return angleY; }
  double getFocDist() const { return focDist; }
  double getDist() const{ return dist; }
  double getBearing() const { return bearing; }
  double getElevation() const { return elevation; }
  int getShootLeft() const { return backLeft; }
  int getShootRight() const { return backRight; }
  int getBackDir() const { return backDir; }
  int getLeftOpening() const { return leftOpening; }
  int getRightOpening() const { return rightOpening; }
  bool shotAvailable() const { return shoot; }
  string toString() const { return getStringFromID(id); }
  const point<double> getFieldLocation() const { return fieldLocation; }
  const double getFieldX() const { return fieldLocation.x; }
  const double getFieldY() const { return fieldLocation.y; }
  const fieldObjectID getID() const { return id; }

  static const double getHeightFromGround(const fieldObjectID id);
  static const bool isBeacon(const FieldObjects * obj);
  static const bool isGoal(const FieldObjects * obj);
  static const bool isArc(const FieldObjects * obj);




 public:
  static const FieldObjects blue_goal_left_post,
    blue_goal_right_post,
    yellow_goal_left_post,
    yellow_goal_right_post,
    yellow_blue_beacon,
    blue_yellow_beacon;
    
  static const double WHITE_HEIGHT_ON_BEACON;

 private:
  Vision *vision;
 
  point <int> leftTop;
  point <int> rightTop;
  point <int> leftBottom;
  point <int> rightBottom;

  double width;
  double height;
  int x;
  int y;
  int centerX;
  int centerY;
  int certainty;
  int distCertainty;
  int backLeft;
  int backRight;
  int backDir;
  int leftOpening;
  int rightOpening;
  bool shoot;
  double angleX;
  double angleY;
  double focDist;
  double dist;
  double bearing;
  double elevation;
  point <double> fieldLocation;
  fieldObjectID id;
};

#endif // FieldObjects_h_DEFINED
