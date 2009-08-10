
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


#ifndef Ball_h_DEFINED
#define Ball_h_DEFINED

#include <stdlib.h>

#include "Common.h"
#include "ifdefs.h"
//#include "NaoPose.h"
#include "VisionStructs.h"
#include "VisionHelpers.h"
#include "Blob.h"
#include "Blobs.h"

class Ball; // forward reference
#include "Threshold.h"
#include "VisualBall.h"
#include "Vision.h"


//constants to constrain blob and point amounts
static const int MAX_BALLS = 400;
static const int MAX_BALL_POINTS = 100;
static const int MAX_NUM_BALL_RUNS = 500; //never use this!
static const int BALL_RUNS_MALLOC_SIZE = 10000;
static const int BAD_VALUE = -10000;
static const int NOISE_SKIPS = 1;

class Ball {
public:
    Ball(Vision* vis, Threshold* thr, int _color);
    virtual ~Ball() {}

    // SETTERS
    void setColor(int c);
    void allocateColorRuns();
	void newRun(int x, int y, int h);

    // Making object
    void init(float s);

    // scan operations
    void vertScan(int x, int y, int dir, int stopper, int c, int c2, stop & scan);
    void horizontalScan(int x, int y, int dir, int stopper, int c, int c2, int l,
                        int r, stop & scan);
    int horizonAt(int x);

    // main methods
    void createBall(int c);

    // ball stuff
    float rightColor(Blob obj, int c);
    float rightHalfColor(Blob obj);
    bool greenCheck(Blob b);
    bool greenSide(Blob b);
    int scanOut(int start_x, int start_y, float slope,int dir);
    int ballNearGreen(Blob b);
    int roundness(Blob b);
    bool badSurround(Blob b);
    bool atBoundary(Blob b);
	void setBallInfo(int w, int h, VisualBall *thisBall);
    int balls(int c, VisualBall *thisBall);

    // sanity checks
    bool rightBlobColor(Blob obj, float per);
    void addPoint(float x, float y);
	bool blobOk(Blob b);

    // debugging methods
    void printBall(Blob b, int c, float p, int o);
    void drawPoint(int x, int y, int c);
    void drawRect(int x, int y, int w, int h, int c);
    void drawBlob(Blob b, int c);
    void drawLine(int x, int y, int x1, int y1, int c);
    void printBlob(Blob b);
    void paintRun(int x,int y, int h, int c);
    void drawRun(const run& run, int c);


private:
    // class pointers
    Vision* vision;
    Threshold* thresh;

    int topSpot; //keeps track of best blob -> global var for debugging purposes
    int color;
    int runsize;
    int biggestRun;
    int maxHeight;
    int maxOfBiggestRun;
    int numberOfRuns;
    int indexOfBiggestRun;
    run* runs;

    Blob *topBlob, zeroBlob;
    //Blob checker, obj, pole, leftBox, rightBox;
    Blobs *blobs;
    int inferredConfidence;
    float slope;
    int occlusion;
    point <int> spot;
    int numPoints;
    float points[MAX_BALL_POINTS*2];

};

#endif // Ball_h_DEFINED
