#ifndef Blob_h_defined
#define Blob_h_defined

#include "Common.h"
#include "ifdefs.h"
#include "NaoPose.h"
#include "VisionStructs.h"
#include "VisionHelpers.h"


class Blob {
public:
    Blob();
    virtual ~Blob() {}

    // SETTERS
	void setLeftTop(point <int> lt) {leftTop = lt;}
	void setRightTop(point <int> rt) {rightTop = rt;}
	void setLeftBottom(point <int> lb) {leftBottom = lb;}
	void setRightBottom(point <int> rb) {rightBottom = rb;}
	void setLeftTopX(int x) {leftTop.x = x;}
	void setLeftTopY(int y) {leftTop.y = y;}
	void setRightTopX(int x) {rightTop.x = x;}
	void setRightTopY(int y) {rightTop.y = y;}
	void setLeftBottomX(int x) {leftBottom.x = x;}
	void setLeftBottomY(int y) {leftBottom.y = y;}
	void setRightBottomX(int x) {rightBottom.x = x;}
	void setRightBottomY(int y) {rightBottom.y = y;}
	void setArea(int a) {area = a;}
	void setPixels(int p) {pixels = p;}

	// GETTERS
	point<int> getLeftTop() {return leftTop;}
	int getLeftTopX() {return leftTop.x;}
	int getLeftTopY() {return leftTop.y;}
	point<int> getRightTop() {return rightTop;}
	int getRightTopX() {return rightTop.x;}
	int getRightTopY() {return rightTop.y;}
	point<int> getLeftBottom() {return leftBottom;}
	int getLeftBottomX() {return leftBottom.x;}
	int getLeftBottomY() {return leftBottom.y;}
	point<int> getRightBottom() {return rightBottom;}
	int getRightBottomX() {return rightBottom.x;}
	int getRightBottomY() {return rightBottom.y;}
	int width();
	int height();
	int getArea();
	int getPixels() {return pixels;}

    // blobbing
	void init();
	void merge(Blob other);

private:
    // bounding coordinates of the blob
    point <int> leftTop;
    point <int> rightTop;
    point <int> leftBottom;
    point <int> rightBottom;
    int pixels; // the total number of correctly colored pixels in our blob
    int area;
};

#endif // Blob_h_defined
