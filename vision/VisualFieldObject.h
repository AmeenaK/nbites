#ifndef VisualFieldObject_hpp_defined
#define VisualFieldObject_hpp_defined

#include <iomanip>
#include <cstdlib>

class VisualFieldObject;

#include "VisualLandmark.h"
#include "ConcreteFieldObject.h"
#include "Utility.h"
#include "Structs.h"

// Values for the Standard Deviation calculations

// This class should eventually inheret from VisualLandmark, once it is
// cleaned a bit
class VisualFieldObject : public VisualLandmark {

public:
    // Construcotrs
    VisualFieldObject(const int _x, const int _y, const float _distance,
                      const float _bearing);
    VisualFieldObject(const fieldObjectID);
    VisualFieldObject();
    // copy constructor
    VisualFieldObject(const VisualFieldObject&);

    // Destructor
    virtual ~VisualFieldObject() {}

    friend std::ostream& operator<< (std::ostream &o,
                                     const VisualFieldObject &l)
        {
            return o << l.toString() << "\tWidth: " << l.width
                     << "\tHeight: " << l.height
                     << "\tDistance: " << l.getDistance()
                     << "\tAngle X: " << l.angleX << "\tAngle Y: " << l.angleY;
        }

    // INITIALIZATION (happens every frame)
    void init();

    void printDebugInfo(FILE * out);

    // SETTERS
    void setWidth(float w) { width = w; }
    void setHeight(float h) { height = h; }
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
    void setAngleX(float aX) { angleX = aX; }
    void setAngleY(float aY) { angleY = aY; }
    void setElevation(float e) { elevation = e; }
    void setFocDist(float fd) { focDist = fd; }
    void setShoot(bool s1) {shoot = s1;}
    void setBackLeft(int x1) {backLeft = x1;}
    void setBackRight(int y1) {backRight = y1;}
    void setBackDir(int x1) {backDir = x1;}
    void setLeftOpening(int op) { leftOpening = op; }
    void setRightOpening(int op) { rightOpening = op; }
    void setPossibleFieldObjects(list <const ConcreteFieldObject *>
                                 _possibleFieldObjects) {
        possibleFieldObjects = _possibleFieldObjects;
    }
    void setDistanceWithSD(float _distance);
    void setBearingWithSD(float _bearing);

    // GETTERS
    float getWidth() const { return width; }
    float getHeight() const { return height; }
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
    float getAngleX() const { return angleX; }
    float getAngleY() const { return angleY; }
    float getFocDist() const { return focDist; }
    float getElevation() const { return elevation; }
    int getShootLeft() const { return backLeft; }
    int getShootRight() const { return backRight; }
    int getBackDir() const { return backDir; }
    int getLeftOpening() const { return leftOpening; }
    int getRightOpening() const { return rightOpening; }
    bool shotAvailable() const { return shoot; }
    string toString() const { return ConcreteFieldObject::getStringFromID(id); }
    const point<float> getFieldLocation() const { return fieldLocation; }
    const float getFieldX() const { return fieldLocation.x; }
    const float getFieldY() const { return fieldLocation.y; }
    const fieldObjectID getID() const { return id; }
    const list <const ConcreteFieldObject *> getPossibleFieldObjects() const {
        return possibleFieldObjects;
    }


private: // Class Variables

    point <int> leftTop;
    point <int> rightTop;
    point <int> leftBottom;
    point <int> rightBottom;

    float width;
    float height;
    int centerX;
    int centerY;
    int backLeft;
    int backRight;
    int backDir;
    int leftOpening;
    int rightOpening;
    bool shoot;
    float angleX;
    float angleY;
    float focDist;
    float elevation;
    point <float> fieldLocation;
    fieldObjectID id;
    // This list will hold all the possibilities for this objects's specific ID
    list <const ConcreteFieldObject *> possibleFieldObjects;

    // Helper Methods
    inline float postDistanceToSD(float _distance) {
        return 0.0496f * exp(0.0271f * _distance);
    }
    inline float postBearingToSD(float _bearing) {
        return M_PI / 8.0f;
    }
};

#endif
