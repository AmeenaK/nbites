/* Vision.cc  -- The Main Vision Module Class file.

   This file does the main initialization for the entire module, including: picking the camera settings, declaring all the class variables, starting AiboConnect TCP Server, amongst other things

   Then, this file contains the main processing loop for the module, called notifyImage(), which basically does all vision processing, localization, behavior control, and communication.

*/

#include "Vision.h" // Vision Class Header File

static byte global_image[IMAGE_BYTE_SIZE];


// Vision Class Constructor
Vision::Vision(Pose *_pose, Profiler *_prof)
  /* jf- commented out until we can remove pointers (change to references)
  : bgrp(this), bglp(this), bgBackstop(this), // blue goal
    ygrp(this), ygrp(this), ygBackstop(this), // yellow goal
    by(this), yb(this),                       // beacons
    blueArc(this), yellowArc(this),           // arcs
    ball(this),                               // orange ball
#ifdef USE_PINK_BALL
    pinkBall(this),                           // pink ball
#endif
    thresh(this),                             // threshold
    pose(p),                                  // pose
    fieldLines(this),                         // fieldLines
    */
  : pose(_pose), profiler(_prof),
    frameNumber(0), id(-1), name(), player(1), colorTable("table.mtb")
{
  // variable initialization

  /* declaring class pointers for field objects, ball, leds, lines*/
  ygrp = new VisualFieldObject(YELLOW_GOAL_RIGHT_POST);
  yglp = new VisualFieldObject(YELLOW_GOAL_LEFT_POST);
  bgrp = new VisualFieldObject(BLUE_GOAL_RIGHT_POST);
  bglp = new VisualFieldObject(BLUE_GOAL_LEFT_POST);
  ygBackstop = new VisualFieldObject();
  bgBackstop = new VisualFieldObject();
  yellowArc = new VisualFieldObject();
  blueArc = new VisualFieldObject();
  yb = new VisualFieldObject(YELLOW_BLUE_BEACON);
  by = new VisualFieldObject(BLUE_YELLOW_BEACON);
  ball = new Ball(this);
  red1 = new VisualFieldObject();
  red2 = new VisualFieldObject();
  navy1 = new VisualFieldObject();
  navy2 = new VisualFieldObject();
#ifdef USE_PINK_BALL
  pinkBall = new Ball(this); //added for pink ball recognition
#endif
  thresh = new Threshold(this, pose);
  fieldLines = new FieldLines(this, pose);
  // pose handled in default contructor

  thresh->setYUV(&global_image[0]);
}

// Vision Class Deconstructor
Vision::~Vision()
{
}

void Vision::copyImage(const byte* image) {
  memcpy(&global_image[0], image, IMAGE_BYTE_SIZE);
  thresh->setYUV(&global_image[0]);
}

void Vision::notifyImage(const byte* image) {
  // Set the current image pointer in Threshold
  thresh->setYUV(image);
  notifyImage();
}

/* notifyImage() -- The Image Loop

   This is the most important loop, ever, really.  This is what the operating system calls when there is a new image.  Here is--in order--what we do next:

   -Do Chromatic Distortion filtering on the Y,Cr,Cb values
         -this is done by the setYUV() method and in CorrectedImage.cc
   -Vision Processing->thresholding and object recognition.
         -this is done in ChownRLE.cc (thresholding) and in ObjectFragments (rle and recognition)
   -Behavior processing
         -processFrame() in PyEmbed.cc, and then in the PyCode brain
   -Handle image arrays if AiboConnect is requesting them
   -Calculate Frames Per Second.

 */
void Vision::notifyImage() {

  // NORMAL VISION LOOP
  PROF_ENTER(profiler, P_VISION);

  frameNumber++;
  // counts the frameNumber
  if (frameNumber > 1000000) frameNumber = 0;

  // Transform joints into pose estimations and horizon line
  PROF_ENTER(profiler, P_TRANSFORM);
  pose->transform();
  PROF_EXIT(profiler, P_TRANSFORM);

  // Perform image correction, thresholding, and object recognition
  thresh->visionLoop();

  PROF_EXIT(profiler, P_VISION);
}

void Vision::setImage(const byte *image) {
  thresh->setYUV(image);
}

std::string Vision::getThreshColor(int _id) {
  switch (_id) {
  case WHITE: return "WHITE";
  case ORANGE: return "ORANGE";
  case BLUE: return "BLUE";
  case GREEN: return "GREEN";
  case YELLOW: return "YELLOW";
  //case PINK: return "PINK";
  case BLACK: return "BLACK";
  case RED: return "RED";
  case NAVY: return "NAVY";
  case GREY: return "GREY";
  case YELLOWWHITE: return "YELLOWWHITE";
  case BLUEGREEN: return "BLUEGREEN";
  case PINK: return "PINK";
  //case BOTHWHITE: return "BOTHWHITE";
  //case TRUE_BLUE: return "TRUE_BLUE";
  default: return "No idea what thresh color you have, mate";
  }
}

std::string Vision::getRobotName() {
  return name;
}



/*******************************|
| Vision visualization methods. |
|*******************************/


/* draBoxes() --
   draws bounding boxes around all the objects we identify:
   -Goals (also draws top and bottom points)
   -Posts
   -Ball
   -Dogs
*/
void Vision::drawBoxes(void) {

  // draw field objects
  if(bglp->getDist() > 0) drawFieldObject(bglp,RED);
  if(bgrp->getDist() > 0) drawFieldObject(bgrp,BLACK);
  if(yglp->getDist() > 0) drawFieldObject(yglp,RED);
  if(ygrp->getDist() > 0) drawFieldObject(ygrp,BLACK);
  if(ygBackstop->getHeight() > 0) drawFieldObject(ygBackstop, BLUE);
  if(bgBackstop->getHeight() > 0) drawFieldObject(bgBackstop, YELLOW);
#if ROBOT(AIBO)
  if(by->getDist() > 0) drawFieldObject(by,GREEN);
  if(yb->getDist() > 0) drawFieldObject(yb,ORANGE);
#endif

  // balls
  // orange
  if(ball->getWidth() > 0)
    drawRect(ball->getX(), ball->getY(),
	     ROUND(ball->getWidth()), ROUND(ball->getHeight()), PINK);
  // pink
#ifdef USE_PINK_BALL
  if(pinkBall->getWidth() > 0)
    drawRect(pinkBall->getX(), pinkBall->getY(),
	     ROUND(pinkBall->getWidth()), ROUND(pinkBall->getHeight()), BLUE);
#endif

  // lines
  drawFieldLines();

  // pose horizon line
  drawLine(pose->getLeftHorizon().x,
	   pose->getLeftHorizon().y,
	   pose->getRightHorizon().x,
	   pose->getRightHorizon().y,
	   BLUE);

  // vision horizon line
  drawPoint(IMAGE_WIDTH/2, thresh->getVisionHorizon(), RED);

} // drawBoxes

// self-explanatory
void Vision::drawFieldObject(VisualFieldObject* obj, int color) {
  drawLine(obj->getLeftTopX(), obj->getLeftTopY(),
	   obj->getRightTopX(), obj->getRightTopY(), color);
  drawLine(obj->getLeftTopX(), obj->getLeftTopY(),
	   obj->getLeftBottomX(), obj->getLeftBottomY(), color);
  drawLine(obj->getRightBottomX(), obj->getRightBottomY(),
	   obj->getRightTopX(), obj->getRightTopY(), color);
  drawLine(obj->getLeftBottomX(), obj->getLeftBottomY(),
	   obj->getRightBottomX(), obj->getRightBottomY(), color);
}

/* drawBox()
   --helper method for drawing rectangles on the thresholded array
   for AiboConnect visualization.
   --takes as input the left,right (x1,x2),bottom,top (y1,y2)
   --the rectangle drawn is a non-filled box.
*/
void Vision::drawBox(int left, int right, int bottom, int top, int c) {
  if (left < 0) {
    left = 0;
  }
  if (top < 0) {
    top = 0;
  }
  int width = right-left;
  int height = bottom-top;

  for (int i = left; i < left + width; i++) {
    if (top >= 0 &&
	top < IMAGE_HEIGHT &&
	i >= 0 &&
	i < IMAGE_WIDTH) {
      thresh->thresholded[top][i] = c;
    }
    if ((top + height) >= 0 &&
	(top + height) < IMAGE_HEIGHT &&
	i >= 0 &&
	i < IMAGE_WIDTH) {
      thresh->thresholded[top + height][i] = c;
    }
  }
  for (int i = top; i < top + height; i++) {
    if (i >= 0 &&
	i < IMAGE_HEIGHT &&
	left >= 0 &&
	left < IMAGE_WIDTH) {
      thresh->thresholded[i][left] = c;
    }
    if (i >= 0 &&
	i < IMAGE_HEIGHT &&
	(left+width) >= 0 &&
	(left+width) < IMAGE_WIDTH) {
      thresh->thresholded[i][left + width] = c;
    }
  }
} // drawBox


/* drawCenters()
   --draws little crosshairs at the center x,y of identified objects.
   includes checks to make sure the crosshairs don't draw off the screen
   (index array out of bounds error producers)
   --draws:
   -ball
   -goals
   -add more if you'd like
*/
void Vision::drawCenters() {
  // draws an X at the ball center X and Y.
  if (ball->getCenterX() >= 2 && ball->getCenterY() >= 2
      && ball->getCenterX() <= (IMAGE_WIDTH-2)
      && ball->getCenterY() <= (IMAGE_HEIGHT-2)) {
    drawPoint(ball->getCenterX(), ball->getCenterY(), YELLOW);
  }

} // drawCenters


/* drawRect()
   --helper method for drawing rectangles on the thresholded array
   for AiboConnect visualization.
   --takes as input the getX,getY (top left x,y coords),
   the width and height of the object,
   and lastly the color of the rectangle you want to use.
   --the rectangle drawn is a non-filled box.
*/
void Vision::drawRect(int left, int top, int width, int height, int c) {
  if (left < 0) {
    width += left;
    left = 0;
  }
  if (top < 0) {
    height += top;
    top = 0;
  }

  for (int i = left; i < left + width; i++) {
    if (top >= 0 && top < IMAGE_HEIGHT && i >= 0 && i < IMAGE_WIDTH) {
      thresh->thresholded[top][i] = c;
    }
    if ((top + height) >= 0 &&
	(top + height) < IMAGE_HEIGHT &&
	i >= 0 &&
	i < IMAGE_WIDTH) {
      thresh->thresholded[top + height][i] = c;
    }
  }
  for (int i = top; i < top + height; i++) {
    if (i >= 0 &&
	i < IMAGE_HEIGHT &&
	left >= 0 &&
	left < IMAGE_WIDTH) {
      thresh->thresholded[i][left] = c;
    }
    if (i >= 0 &&
	i < IMAGE_HEIGHT &&
	(left+width) >= 0 &&
	(left+width) < IMAGE_WIDTH) {
      thresh->thresholded[i][left + width] = c;
    }
  }
#if ROBOT(NAO)
  left--;
  width+=2;
  height+=2;
  top--;
  if (left < 0) {
    width += left;
    left = 0;
  }
  if (top < 0) {
    height += top;
    top = 0;
  }
  for (int i = left; i < left + width; i++) {
    if (top >= 0 && top < IMAGE_HEIGHT && i >= 0 && i < IMAGE_WIDTH) {
      thresh->thresholded[top][i] = c;
    }
    if ((top + height) >= 0 &&
	(top + height) < IMAGE_HEIGHT &&
	i >= 0 &&
	i < IMAGE_WIDTH) {
      thresh->thresholded[top + height][i] = c;
    }
  }
  for (int i = top; i < top + height; i++) {
    if (i >= 0 &&
	i < IMAGE_HEIGHT &&
	left >= 0 &&
	left < IMAGE_WIDTH) {
      thresh->thresholded[i][left] = c;
    }
    if (i >= 0 &&
	i < IMAGE_HEIGHT &&
	(left+width) >= 0 &&
	(left+width) < IMAGE_WIDTH) {
      thresh->thresholded[i][left + width] = c;
    }
  }
#endif
} // drawRect

/* drawLine()
   --helper visualization method for drawing a line given two points and a color
*/
void Vision::drawLine(int x, int y, int x1, int y1, int c) {
  float slope = (float)(y - y1) / (float)(x - x1);
  int sign = 1;
  if ((abs(y - y1)) > (abs(x - x1))) {
    slope = 1.0 / slope;
    if (y > y1) sign = -1;
    for (int i = y; i != y1; i += sign) {
      int newx = x + (int)(slope * (i - y));
      if (newx >= 0 && newx < IMAGE_WIDTH && i >= 0 && i < IMAGE_HEIGHT)
	thresh->thresholded[i][newx] = c;
    }
  } else if (slope != 0) {
    //slope = 1.0 / slope;
    if (x > x1) sign = -1;
    for (int i = x; i != x1; i += sign) {
      int newy = y + (int)(slope * (i - x));
      if (newy >= 0 && newy < IMAGE_HEIGHT && i >= 0 && i < IMAGE_WIDTH)
	thresh->thresholded[newy][i] = c;
    }
  }
  else if (slope == 0) {
    int startX = min(x, x1);
    int endX = max(x, x1);
    for (int i = startX; i <= endX; i++) {
      if (y >= 0 && y < IMAGE_HEIGHT && i >= 0 && i < IMAGE_WIDTH) {
	thresh->thresholded[y][i] = c;
      }
    }
  }
#if ROBOT(NAO)
  y--;
  y1--;
  x--;
  x1--;
  if ((abs(y - y1)) > (abs(x - x1))) {
    slope = 1.0 / slope;
    if (y > y1) sign = -1;
    for (int i = y; i != y1; i += sign) {
      int newx = x + (int)(slope * (i - y));
      if (newx >= 0 && newx < IMAGE_WIDTH && i >= 0 && i < IMAGE_HEIGHT)
	thresh->thresholded[i][newx] = c;
    }
  } else if (slope != 0) {
    //slope = 1.0 / slope;
    if (x > x1) sign = -1;
    for (int i = x; i != x1; i += sign) {
      int newy = y + (int)(slope * (i - x));
      if (newy >= 0 && newy < IMAGE_HEIGHT && i >= 0 && i < IMAGE_WIDTH)
	thresh->thresholded[newy][i] = c;
    }
  }
  else if (slope == 0) {
    int startX = min(x, x1);
    int endX = max(x, x1);
    for (int i = startX; i <= endX; i++) {
      if (y >= 0 && y < IMAGE_HEIGHT && i >= 0 && i < IMAGE_WIDTH) {
	thresh->thresholded[y][i] = c;
      }
    }
  }
#endif
}

// Convenience method to draw a VisualLine to the screen.
void Vision::drawLine(const VisualLine &line, const int color) {
  drawLine(line.start.x, line.start.y, line.end.x, line.end.y, color);
}

/* drawPoint()
   -draws a crosshair or a 'point' at some given x, y, and with a given color
*/
void Vision::drawPoint(int x, int y, int c) {
  if (y > 0 && x > 0 && y < (IMAGE_HEIGHT) && x < (IMAGE_WIDTH)) {
    thresh->thresholded[y][x] = c;
  }if (y+1 > 0 && x > 0 && y+1 < (IMAGE_HEIGHT) && x < (IMAGE_WIDTH)) {
    thresh->thresholded[y+1][x] = c;
  }if (y+2 > 0 && x > 0 && y+2 < (IMAGE_HEIGHT) && x < (IMAGE_WIDTH)) {
    thresh->thresholded[y+2][x] = c;
  }if (y-1 > 0 && x > 0 && y-1 < (IMAGE_HEIGHT) && x < (IMAGE_WIDTH)) {
    thresh->thresholded[y-1][x] = c;
  }if (y-2 > 0 && x > 0 && y-2 < (IMAGE_HEIGHT) && x < (IMAGE_WIDTH)) {
    thresh->thresholded[y-2][x] = c;
  }if (y > 0 && x+1 > 0 && y < (IMAGE_HEIGHT) && x+1 < (IMAGE_WIDTH)) {
    thresh->thresholded[y][x+1] = c;
  }if (y > 0 && x+2 > 0 && y < (IMAGE_HEIGHT) && x+2 < (IMAGE_WIDTH)) {
    thresh->thresholded[y][x+2] = c;
  }if (y > 0 && x-1 > 0 && y < (IMAGE_HEIGHT) && x-1 < (IMAGE_WIDTH)) {
    thresh->thresholded[y][x-1] = c;
  }if (y > 0 && x-2 > 0 && y < (IMAGE_HEIGHT) && x-2 < (IMAGE_WIDTH)) {
    thresh->thresholded[y][x-2] = c;
  }
}

// Convenience method to draw linePoints
void Vision::drawPoint(const linePoint &p, const int color) {
  drawPoint(p.x, p.y, color);
}
// Convenience method to draw corners
void Vision::drawPoint(const VisualCorner &c, const int color) {
  drawPoint(c.getX(), c.getY(), color);
}

/* drawVerticalLine()
   --helper method to visualize a vertical line on the screen given a x-value
   --given a color as well
*/
void Vision::drawVerticalLine(int x, int c) {
  if (x >= 0 && x < IMAGE_WIDTH) {
    for (int i = 0; i < IMAGE_HEIGHT; i++) {
      thresh->thresholded[i][x] = c;
    }
  }
}

/* drawHorizontalLine()
   --helper method to visualize a vertical line on the screen given a x-value
   --given a color as well
*/
void Vision::drawHorizontalLine(int y, int c) {
  if (y >= 0 && y < IMAGE_HEIGHT) {
    for (int i = 0; i < IMAGE_WIDTH; i++) {
      thresh->thresholded[y][i] = c;
#if ROBOT(NAO)
      if (y + 1 < IMAGE_HEIGHT - 1) {
	thresh->thresholded[y+1][i] = c;
      }
#endif
    }
  }
}

// Draws gigantic lines across the screen centered on one x,y coordinate
void Vision::drawCrossHairs(int x, int y, int c) {
  drawHorizontalLine(y,c);
  drawVerticalLine(x,c);
}


/* drawVerticalLine()
   --helper method to visualize a single point on the screen given a x-value
   --given a color as well
   --use drawPoint() if you really want to see the point well.
*/
void Vision::drawDot(int x, int y, int c) {
  if (y > 0 && x > 0 && y < (IMAGE_HEIGHT) && x < (IMAGE_WIDTH)) {
    thresh->thresholded[y][x] = c;
  }
}

void Vision::drawFieldLines() {

  const vector<VisualLine>* lines = fieldLines->getLines();

  for (vector<VisualLine>::const_iterator i = lines->begin();
       i != lines->end(); i++) {
    drawLine(*i, BLUE);

    // Draw all the line points in the line
    for (vector<linePoint>::const_iterator j = i->points.begin();
         j != i->points.end(); j++) {
      // Vertically found = black
      if (j->foundWithScan == VERTICAL) {
        drawPoint(*j, BLACK);
      }
      // Horizontally found = red
      else {
        drawPoint(*j, RED);
      }
    }
  }

  const list <linePoint>* unusedPoints = fieldLines->getUnusedPoints();
  for (list <linePoint>::const_iterator i = unusedPoints->begin();
       i != unusedPoints->end(); i++) {
    // Unused vertical = PINK
    if (i->foundWithScan == VERTICAL) {
      drawPoint(*i, PINK);
    }
    // Unused horizontal = Yellow
    else {
      drawPoint(*i, YELLOW);
    }
  }

  const list <VisualCorner>* corners = fieldLines->getCorners();
  for (list <VisualCorner>::const_iterator i = corners->begin();
       i != corners->end(); i++) {
    drawPoint(*i, ORANGE);
  }
}
