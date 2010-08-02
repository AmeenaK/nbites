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
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
// GNU Lesser Public License for more details.
//
// You should have received a copy of the GNU General Public License
// and the GNU Lesser Public License along with Man.  If not, see
// <http://www.gnu.org/licenses/>.


/*
 * This is where we do all major ball related processing.  For now we
 * still do run length encoding to find candidate balls.  What we do
 * is collect up all of the candidate blobs and run them through a
 * series of initial sanity checks to whittle the list down to a smaller
 * number.	From that list we then start with the biggest candidate
 * ball and run more sanity checks until we find a ball that we
 * are satisfied with.	Basically our sanity checks revolve around:
 * color - the ball should be mostly orange, red is dangerous
 * shape - the ball should be pretty round
 * location - the ball should be on the field
 * distance - the perceived distance of the ball should match where
 *			  it is in our field of view
 */

#include <iostream>
#include "Ball.h"
#include "debug.h"
#include <vector>

using namespace std;

// Ball constants
// EXAMINED: look at this switch - SMALLBALLDIM
static const int SMALLBALLDIM = 3; // below this size balls are considered small
static const int SMALLBALL = SMALLBALLDIM * SMALLBALLDIM;
// ratio of width/height worse than this is a very bad sign
static const float BALLTOOFAT = 1.5f;
// ditto
static const float BALLTOOTHIN = 0.75f;
// however, if the ball is occluded we can go thinner
static const float OCCLUDEDTHIN = 0.2f;
// or fatter
static const float OCCLUDEDFAT = 4.0f;
static const float MIDFAT = 3.0f;
static const float MIDTHIN = 0.3f;
// at least this much of the blob should be orange normally
static const float MINORANGEPERCENTSMALL = 0.5f;
static const float MINORANGEPERCENT = 0.5f;
static const float MINGOODBALL = 0.5f;
static const float MAXGOODBALL = 3.0f;
static const int BIGAREA = 400;
static const int BIGGERAREA = 600;
static const float FATBALL = 2.0f;
static const float THINBALL = 0.5f;

static const int DIST_POINT_FUDGE = 5;

//previous constants inserted from .h class


Ball::Ball(Vision* vis, Threshold* thr, Field* fie, int _color)
: vision(vis), thresh(thr), field(fie), color(_color), runsize(1)
{
	blobs = new Blobs(MAX_BALLS);
	init(0.0);
	allocateColorRuns();
}


/* Initialize the data structure.
 * @param s		the slope corresponding to the robot's head tilt
 */
void Ball::init(float s)
{
	blobs->init();
	slope = s;
	biggestRun = 0;
	maxHeight = IMAGE_HEIGHT;
	maxOfBiggestRun = 0L;
	numberOfRuns = 0;
	indexOfBiggestRun = 0;
	numPoints = 0;
}

/* This is the entry  point ball recognition in Threshold.cc
 * @param  h			field horizon
 * @return				always 0
 */

void Ball::createBall(int h) {
    // start by building blobs
	if (numberOfRuns > 1) {
		for (int i = 0; i < numberOfRuns; i++) {
			// search for contiguous blocks
			int nextX = runs[i].x;
			int nextY = runs[i].y;
			int nextH = runs[i].h;
			blobs->blobIt(nextX, nextY, nextH);
		}
	}
	balls(h, vision->ball);
}

/*  Before we just take the biggest blob, we do some initial screening
    based upon basic blob information.  The main things we look at here
    are the color of the blob and its size.  When we find a blob that
    doesn't meet our criteria we simply re-initialize it so it will
    no longer be considered.
 */
void Ball::preScreenBlobsBasedOnSizeAndColor() {
    const int MIN_AREA = 12;
    const int MAX_AREA = 1000;

	// pre-screen blobs that don't meet our criteria
	for (int i = 0; i < blobs->number(); i++) {
		int ar = blobs->get(i).getArea();
		float perc = rightColor(blobs->get(i), ORANGE);
		int diam = max(blobs->get(i).width(), blobs->get(i).height());
		float minpercent = MINORANGEPERCENT;
		// For now we are going to allow very small balls to be a bit less orange
		// obviously this is dangerous, so we'll have to keep an eye on it.
		if (ar < MIN_AREA * 3) {
			minpercent = MINORANGEPERCENTSMALL;
		}
		if (blobs->get(i).getArea() > 0) {
			if (blobs->get(i).getLeftBottomY() + diam <
					horizonAt(blobs->get(i).getLeftTopX())) {
				blobs->init(i);
				if (BALLDEBUG) {
					cout << "Screened one for horizon problems " << endl;
					drawBlob(blobs->get(i), WHITE);
				}
			} else if (ar > MIN_AREA && perc > minpercent) {
				// don't do anything
				if (BALLDEBUG) {
					cout << "Candidate ball " << endl;
					printBlob(blobs->get(i));
				}
			} else if (ar > MAX_AREA && rightHalfColor(blobs->get(i)) > minpercent)
			{
				if (BALLDEBUG) {
					cout << "Candidate ball " << endl;
					printBlob(blobs->get(i));
				}
			} else {
				if (BALLDEBUG) {
					drawBlob(blobs->get(i), BLACK);
					printBlob(blobs->get(i));
					if (perc < minpercent) {
						cout << "Screened one for not being orange enough, its percentage is "
								<< perc << endl;
					} else {
						cout << "Screened one for being too small - its area is " << ar << endl;
					}
				}
				blobs->init(i);
			}
		}
	}
}

/* Do more sanity checks on the ball returning false if any fail.
   @return          whether the ball passes the sanity checks
 */
bool Ball::sanityChecks(int w, int h, estimate e, VisualBall * thisBall) {
    const float DISTANCE_MISMATCH = 50.0f;
	const float PIXACC = 300;
	const int HORIZON_THRESHOLD = 30;

    float distanceDifference = fabs(e.dist - thisBall->getDistance());
    int horb = horizonAt(topBlob->getLeftBottomX());

    if (badSurround(*topBlob)) {
        if (BALLDEBUG) {
            drawBlob(*topBlob, BLACK);
            cout << "Screening for lack of green and bad surround" << endl;
        }
        topBlob->init();
        thisBall->init();
        return false;
    } else if (distanceDifference > DISTANCE_MISMATCH &&
               (e.dist *2 <  thisBall->getDistance() ||
                thisBall->getDistance() * 2 < e.dist)
               && e.dist < PIXACC && e.dist > 0) {
        if (BALLDEBUG) {
            cout << "Screening due to distance mismatch " << e.dist <<
                " " << thisBall->getDistance() << endl;
        }
        thisBall->init();
        topBlob->init();
        return false;
    } else if (w < SMALLBALLDIM || h < SMALLBALLDIM) {
        // small balls should be near the horizon - this check makes extra sure
        if (topBlob->getLeftBottomY() > horb + HORIZON_THRESHOLD) {
            if (BALLDEBUG) {
                cout << "Screening small ball for horizon" << endl;
            }
            thisBall->init();
            topBlob->init();
            return false;
        }
    }
    return true;
}


/* See if there is a ball onscreen.	 Basically we get all of the orange blobs
 * and test them for viability.	 Once we've screened all of the obviously bad
 * ones we then pick the biggest one and check it some more.
 *
 * @param  horizon	 the horizon intercept
 * @param  thisBall	 the ball object
 * @return			 we always return 0 - an artifact of other methods
 */
int Ball::balls(int horizon, VisualBall *thisBall)
{
	const int PIX_EST_DIV = 3;
	occlusion = NOOCCLUSION;
	int w, h;	// width and height of potential ball
	estimate e; // pix estimate of ball's distance

    preScreenBlobsBasedOnSizeAndColor();
    // loop through the blobs from biggest to smallest until we find a ball
	do {
		topBlob = blobs->getTopAndMerge(horizon);
        // the conditions when we know we don't have a ball
		if (topBlob == NULL || !blobOk(*topBlob)) {
            return 0;
        }
		w = topBlob->width();
		h = topBlob->height();
		const float BALL_REAL_HEIGHT = 6.5f;
		e = vision->pose->pixEstimate(topBlob->getLeftTopX() + (w / 2),
									  topBlob->getLeftTopY() + 2 * h / PIX_EST_DIV,
									  ORANGE_BALL_RADIUS);
        // SORT OUT BALL INFORMATION
        setOcclusionInformation();
        setBallInfo(w, h, thisBall, e);
    } while (!sanityChecks(w, h, e, thisBall));

    // last second adjustment for non-square balls
    if (ballIsClose(thisBall) && ballIsNotSquare(h, w)) {
        checkForReflections(h, w, thisBall, e);
    }
    if (BALLDEBUG) {
        cout << "Vision found ball " << endl;
        cout << topBlob->getLeftTopX() << " " << topBlob->getLeftTopY()
             << " " <<
            w << " " << h << " " << e.dist << endl;
    }
	if (BALLDISTDEBUG) {
		estimate es;
		es = vision->pose->pixEstimate(topBlob->getLeftTopX() + topBlob->width() /
									   2, topBlob->getLeftTopY() + 2
									   * topBlob->height() / PIX_EST_DIV, ORANGE_BALL_RADIUS);
		cout << "Distance is " << thisBall->getDistance() << " " <<
				thisBall->getFocDist() << " " << es.dist << endl;
		cout<< "Radius"<<thisBall->getRadius()<<endl;
	}
	return 0;
}

/* Determines on which side the ball is obviously occluded.
 */
void Ball::setOcclusionInformation() {
    if (topBlob->getLeftBottomY() > IMAGE_HEIGHT - 3) {
        occlusion = BOTTOMOCCLUSION;
    }
    if (topBlob->getLeftTopY() < 1) {
        occlusion *= TOPOCCLUSION;
    }
    if (topBlob->getLeftTopX() < 1) {
        occlusion *= LEFTOCCLUSION;
    }
    if (topBlob->getRightTopX() > IMAGE_WIDTH - 2) {
        occlusion *= RIGHTOCCLUSION;
    }
}

/* Sometimes we get a reflection off a post or other robot and it skews how
   we see the ball.  A good clue is that the ball is close and isn't very
   square.  So when that happens we see if we can improve on its basic
   squareness.  We do that here.
   @param h        the ball height
   @param w        the ball width
   @param thisBall the ball
   @param e        pixEstimated distance to ball
 */
void Ball::checkForReflections(int h, int w, VisualBall * thisBall,
                               estimate e) {
    // we probably have misidentified the distance see if we can fix it.
    if (BALLDISTDEBUG) {
        cout << "Detected bad ball distance - trying to fix " << w <<
            " " << h << endl;
    }
    // generally the reflections are over or under the ball
    if (h > w) {
        // scan the sides to find the real sides
        int count = -2;
        if (topBlob->getRightTopX() - h > 0) {
            for (int i = topBlob->getRightTopX() - h; i < IMAGE_WIDTH - 1;
                 i++) {
                for (int j = topBlob->getLeftTopY();
                     j < topBlob->getLeftBottomY(); j++) {
                    if (thresh->thresholded[j][i] == ORANGE) {
                        topBlob->setRightTopX(i);
                        j = IMAGE_HEIGHT;
                        i = IMAGE_WIDTH;
                    }
                }
                count++;
            }
        }
        if (topBlob->getLeftTopX() + h < IMAGE_WIDTH) {
            for (int i = topBlob->getLeftTopX() + h; i > -1; i--) {
                for (int j = topBlob->getLeftTopY();
                     j < topBlob->getLeftBottomY(); j++) {
                    if (thresh->thresholded[j][i] == ORANGE) {
                        topBlob->setRightTopX(i);
                        j = IMAGE_HEIGHT;
                        i = -1;
                    }
                }
                count++;
            }
        }
        if (count > 1) {
            if (BALLDISTDEBUG) {
                cout << "Resetting ball dimensions.	 Count was " << count
                     << endl;
            }
            setBallInfo(w, w, thisBall, e);
        }
    }
    thisBall->setDistanceEst(e);
}

/* Returns true when the ball is close (3/4 of a meter).
   @param thisBall       the ball
   @return               whether it is within the prescribed distance
 */

bool Ball::ballIsClose(VisualBall * thisBall) {
    return thisBall->getDistance() < 75.0f;
}

/* Returns true when the height and width are not a good match.
   @param h      ball height
   @param w      ball width
   @return       whether the ball is square or not
 */
bool Ball::ballIsNotSquare(int h, int w) {
    return abs(h - w) > 3;
}

/* Set the primary color.  Depending on the color, we have different space needs
 * @param c		   the color
 */
void Ball::setColor(int c)
{
	const int RUN_VALUES = 3;			// x, y, and h
	const int RUNS_PER_LINE = 5;
	const int RUNS_PER_SCANLINE = 15;

	runsize = 1;
	int run_num = RUN_VALUES;
	color = c;
	runsize = BALL_RUNS_MALLOC_SIZE; //max number of runs
	run_num = runsize * RUN_VALUES;
	runs = (run*)malloc(sizeof(run) * run_num);
}


/* Allocate the required amount of memory dependent on the primary color
 */
void Ball::allocateColorRuns()
{
	const int RUN_VALUES = 3;		  // x, y and h
	const int RUNS_PER_SCANLINE = 15;
	const int RUNS_PER_LINE = 5;

	int run_num = RUN_VALUES;
	// depending on the color we have more or fewer runs available
	runsize = BALL_RUNS_MALLOC_SIZE; //max number of runs
	run_num = runsize * RUN_VALUES;
	runs = (run*)malloc(sizeof(run) * run_num);
}

/* Adds a new run to the basic data structure.

   runs structure contains:
   -x of start column
   -y of start column
   -height of run

   @param x		x value of run
   @param y		y value of top of run
   @param h		height of run
 */
void Ball::newRun(int x, int y, int h)
{
	const int RUN_VALUES = 3;	 // x, y, and h of course

	if (numberOfRuns < runsize) {
		int last = numberOfRuns - 1;
		// skip over noise --- jumps over two pixel noise currently.
		//HW--added CONSTANT for noise jumps.
		if (last > 0 && runs[last].x == x &&
				(runs[last].y - (y + h) <= NOISE_SKIPS)) {
			runs[last].h += runs[last].y - y; // merge run lengths
			runs[last].y = y; // reset the new y val
			h = runs[last].h;
			numberOfRuns--; // don't count this merge as a new run
		} else {
			runs[numberOfRuns].x = x;
			runs[numberOfRuns].y = y;
			runs[numberOfRuns].h = h;
		}

		if (h > biggestRun) { // tracking largest run
			biggestRun = h;
			maxOfBiggestRun = y;
			indexOfBiggestRun = numberOfRuns * RUN_VALUES;
		}
		if (y < maxHeight) { // we're counting backwards
			maxHeight = y;
		}
		numberOfRuns++;
	}else{
		if(color == ORANGE) {
			print("WARNING!!!: INSUFFICIENT MEMORY ALLOCATED ORANGE RUNS");
		}
		//cout << "Too many runs " << color << endl;
	}
}


/* Scan from the point along the line until you have hit "stopper" points that
 * aren't color "c" return the last good point found and how many good and bad
 * points seen.	 Though this is a void function it actually "returns"
 * information in the scan variable. scan.x and scan.y represent the finish
 * points of the line (last point of appropriate color) and bad and good
 * represent how many bad and good pixels (pixels that are of the right color
 * or not) along the way.
 *
 * Note:  Since we are doing this for balls we are always scanning along
 * the true vertical!  This is different than for other classes.
 * @param x		   the x point to start at
 * @param y		   the y point to start at
 * @param dir	   the direction of the scan (positive or negative)
 * @param stopper  how many incorrectly colored pixels we can live with
 * @param c		   color we are most interested in
 * @param c2	   soft color that could also work
 */
void Ball::vertScan(int x, int y, int dir, int stopper, int c,
					int c2, stop &scan)
{
	scan.good = 0;
	scan.bad = 0;
	scan.x = x;
	scan.y = y;
	int bad = 0;
	int good = 0;
	int startX = x;
	int startY = y;
	int run = 1;
	int width = IMAGE_WIDTH;
	int height = IMAGE_HEIGHT;
	// go until we hit enough bad pixels
	for ( ; x > -1 && y > -1 && x < width && y < height && bad < stopper; ) {
		//cout << "Vert scan " << x << " " << y << endl;
		// if it is the color we're looking for - good
		if (thresh->thresholded[y][x] == c || thresh->thresholded[y][x] == c2) {
			good++;
			run++;
			if (run > 1) {
				scan.x = x;
				scan.y = y;
			}
		} else {
			bad++;
			run = 0;
		}
		y = y + dir;
	}
	scan.bad = bad;
	scan.good = good;
	//cout << " out vert " << endl;
}

/* Scan from the point along the line until you have hit "stopper" points that
 * aren't color "c"
 * return the last good point found and how many good and bad points seen.
 * Though this is a void function it actually "returns" information in the scan
 * variable. scan.x and scan.y represent the finish points of the line (last
 * point of appropriate color) and bad and good represent how many bad and good
 * pixels (pixels that are of the right color or not) along the way.
 *
 * Note always horizontal, since we are scanning with regard to ball.
 *
 * @param x			 the x point to start at
 * @param y			 the y point to start at
 * @param dir		 the direction of the scan (positive or negative)
 * @param stopper	 how many incorrectly colored pixels we can live with
 * @param c			 color we are most interested in
 * @param c2		 soft color that could also work
 * @param leftBound	 furthest left we can go
 * @param rightBound further right we can go
 */
void Ball::horizontalScan(int x, int y, int dir, int stopper, int c,
						  int c2, int leftBound, int rightBound,
						  stop & scan) {
	scan.good = 0;
	scan.bad = 0;
	scan.x = x;
	scan.y = y;
	int bad = 0;
	int good = 0;
	int run = 0;
	int startX = x;
	int startY = y;
	int height = IMAGE_HEIGHT;
	// go until we hit enough bad pixels
	for ( ; x > leftBound && y > -1 && x < rightBound && x < IMAGE_WIDTH
	&& y < height && bad < stopper; ) {
		if (thresh->thresholded[y][x] == c || thresh->thresholded[y][x] == c2) {
			// if it is either of the colors we're looking for - good
			good++;
			run++;
			if (run > 1) {
				scan.x = x;
				scan.y = y;
			}
		} else {
			bad++;
			run = 0;
		}
		x = x + dir;
	}
	scan.bad = bad;
	scan.good = good;
	//cout << "return with " << temp.x << endl;
}

int Ball::horizonAt(int x) {
	return field->horizonAt(x);
}


/*	Normally we want our balls to be orange and can just check the number of
 * pixels within the blob
 * that are orange.	 However, sometimes the balls are occluded.
 * If we have a nice big orange blob,
 * but it doesn't seem orange enough it might be occluded.	So we look at
 * different halves of the blob
 * to see if one of them is properly orange.
 * @param tempobj	   the current ball candidate
 * @return			   the best percentage we found
 */
// only called on really big orange blobs
float Ball::rightHalfColor(Blob tempobj)
{
	const float COLOR_THRESH = 0.15f;
	const float POOR_VALUE = 0.10f;

	int x = tempobj.getLeftTopX();
	int y = tempobj.getLeftTopY();
	int spanY = tempobj.height() - 1;
	int spanX = tempobj.width() - 1;
	int good = 0, good1 = 0, good2 = 0;
	int pix;
	if (rightColor(tempobj, ORANGE) < COLOR_THRESH) return POOR_VALUE;
	for (int i = spanY / 2; i < spanY; i++) {
		for (int j = 0; j < spanX; j++) {
			pix = thresh->thresholded[y + i][x + j];
			if (y + i > -1 && x + j > -1 && (y + i) < IMAGE_HEIGHT &&
					x + j < IMAGE_WIDTH && (pix == ORANGE || pix == ORANGERED ||
							pix == ORANGEYELLOW)) {
				good++;
			}
		}
	}
	for (int i = 0; i < spanY; i++) {
		for (int j = 0; j < spanX / 2; j++) {
			pix = thresh->thresholded[y + i][x + j];
			if (y + i > -1 && x + j > -1 && (y + i) < IMAGE_HEIGHT &&
					x + j < IMAGE_WIDTH && (pix == ORANGE || pix == ORANGERED ||
							pix == ORANGEYELLOW)) {
				good1++;
			}
		}
	}
	for (int i = 0; i < spanY; i++) {
		for (int j = spanX / 2; j < spanX; j++) {
			pix = thresh->thresholded[y + i][x + j];
			if (y + i > -1 && x + j > -1 && (y + i) < IMAGE_HEIGHT &&
					x + j < IMAGE_WIDTH && (pix == ORANGE || pix == ORANGERED ||
							pix == ORANGEYELLOW)) {
				good2++;
			}
		}
	}
	if (BALLDEBUG) {
		cout << "Checking half color " << good << " " << good1 << " " <<
				good2 << " " << (spanX * spanY / 2) << endl;
	}
	float percent = (float)max(max(good, good1), good2) /
			(float) (spanX * spanY / 2);
	return percent;
}

/* Checks out how much of the current blob is orange.
 * Also looks for too much red.
 * @param tempobj	  the candidate ball blob
 * @param col		  ???
 * @return			  the percentage (unless a special situation occurred)
 */

float Ball::rightColor(Blob tempobj, int col)
{
	const int MIN_BLOB_SIZE = 1000;
	const float RED_PERCENT = 0.10f;
	const float ORANGE_PERCENT = 0.20f;
	const float ORANGEYELLOW_PERCENT = 0.40f;
	const float GOOD_PERCENT = 0.65f;

	int x = tempobj.getLeftTopX();
	int y = tempobj.getLeftTopY();
	int spanY = tempobj.height();
	int spanX = tempobj.width();
	if (spanX < 2 || spanY < 2) {
        return false;
    }
	int good = 0;
	int ogood = 0;
	int orgood = 0;
	int oygood = 0;
	int red = 0;
	int pink = 0;
	for (int i = 0; i < spanY; i++) {
		for (int j = 0; j < spanX; j++) {
			if (y + i > -1 && x + j > -1 && (y + i) < IMAGE_HEIGHT &&
					x + j < IMAGE_WIDTH) {
				int pix = thresh->thresholded[y + i][x + j];
				if (pix == ORANGE || pix == ORANGERED ||
						pix == ORANGEYELLOW) {
					good++;
					if (pix == ORANGE)
						ogood++;
					else if (pix == ORANGEYELLOW)
						oygood++;
					else
						orgood++;
				} else if (pix == RED) {
					red++;
				}
			}
		}
	}
	// here's a big hack - if we have a ton of orange, let's say it is enough
	// unless the percentage is really low
	if (BALLDEBUG) {
		cout << "Orange " << ogood << " ORed " << orgood << " " << red << " OYel "
				<< oygood << " " << pink << " " << tempobj.getArea() << endl;
	}
	if (tempobj.getArea() > MIN_BLOB_SIZE) {
        return (float) good / (float) tempobj.getArea();
    }
	// pixels should be orange
	if (red > static_cast<float>(spanX * spanY) * RED_PERCENT) {
		if (BALLDEBUG) {
			cout << "Too much red" << endl;
        }
		return RED_PERCENT;
	}
	if (tempobj.getArea() > MIN_BLOB_SIZE &&
        ogood + oygood > (static_cast<float>(spanX * spanY) *
                          ORANGEYELLOW_PERCENT) &&
        good < ( static_cast<float>(spanX * spanY) * GOOD_PERCENT)) {
		return GOOD_PERCENT;
    }
	float percent = (float)good / (float) (spanX * spanY);
	if (col == GREEN) {
		return (float)good;
    }
	return percent;
}

/*	When we're looking for balls it is helpful if they are surrounded by green.
 * The best place to look is underneath.  So let's do that.
 * @param b	   the potential ball
 * @return	   did we find some green?
 */
bool Ball::greenCheck(Blob b)
{
	const int ERROR_TOLERANCE = 5;
	const int EXTRA_LINES = 10;
	const int MAX_BAD_PIXELS = 4;

	if (b.getRightBottomY() >= IMAGE_HEIGHT - 1 ||
			b.getLeftBottomY() >= IMAGE_HEIGHT-1)
		return true;
	if (b.width() > IMAGE_WIDTH / 2) return true;
	int w = b.width();
	int y = 0;
	int x = b.getLeftBottomX();
	stop scan;
	for (int i = 0; i < w; i+= 2) {
		y = b.getLeftBottomY();
		vertScan(x + i, y, 1, ERROR_TOLERANCE, GREEN, GREEN, scan);
		if (scan.good > 1) {
			return true;
        }
	}
	// try one more in case its a white line
	int bad = 0;
	for (int i = 0; i < EXTRA_LINES && bad < MAX_BAD_PIXELS; i++) {
		int pix = thresh->thresholded[min(IMAGE_HEIGHT - 1,
										  b.getLeftBottomY() + i)][x];
		if (pix == GREEN) {
            return true;
        }
		if (pix != WHITE) {
            bad++;
        }
	}
	return false;
}

/*	It probably goes without saying that the ideal ball is round.  So let's see
 * how round our current candidate is.	Among other things we check its
 * height/width ratio (should be about 1) and where the orange is (shouldn't
 * be in the corners, should be in the middle)
 * TODO:  This needs LOTS of work.	Especially if we move to nontraditional
 * colors or multi-colored balls.
 * @param b		 the candidate ball
 * @return		 a constant result - BAD_VALUE, or 0 for round
 */

int	 Ball::roundness(Blob b)
{
	const int IMAGE_EDGE = 3;
	const int BIG_ENOUGH = 4;
	const int TOO_BIG_TO_CHECK = 20;
	const int WIDTH_AT_SCREEN_BOTTOM = 15;
	const float RATIO_TO_INT = 10.0f;
	const float CORNER_CHUNK_DIV = 6.0f;

	int w = b.width();
	int h = b.height();
	int x = b.getLeftTopX();
	int y = b.getLeftTopY();
	float ratio = static_cast<float>(w) / static_cast<float>(h);
	int r = 10;

	if ((h < SMALLBALLDIM && w < SMALLBALLDIM && ratio > BALLTOOTHIN &&
			ratio < BALLTOOFAT)) {
	} else if (ratio > THINBALL && ratio < FATBALL) {
	} else if (y + h > IMAGE_HEIGHT - IMAGE_EDGE || x == 0 || (x + w) >
	IMAGE_WIDTH - 2 || y == 0) {
		if (BALLDEBUG)
			cout << "Checking ratio on occluded ball:  " << ratio << endl;
		// we're on an edge so allow for streching
		if (h > BIG_ENOUGH && w > BIG_ENOUGH && (y + h > IMAGE_HEIGHT - 2 ||
				y == 0) &&
				ratio < MIDFAT && ratio > 1) {
			// then sides
		} else if (h > BIG_ENOUGH && w > BIG_ENOUGH
				&& (x == 0 || x + w > IMAGE_WIDTH - 2)
				&& ratio > MIDTHIN && ratio < 1) {
		} else if ((h > TOO_BIG_TO_CHECK || w > TOO_BIG_TO_CHECK)
				&& (ratio > OCCLUDEDTHIN && ratio < OCCLUDEDFAT) ) {
			// when we have big slivers then allow for extra
		} else if (b.getLeftBottomY() > IMAGE_HEIGHT - IMAGE_EDGE &&
				w > WIDTH_AT_SCREEN_BOTTOM) {
			// the bottom is a really special case
		} else {
			if (BALLDEBUG)
				//cout << "Screening for ratios" << endl;
				return BAD_VALUE;
		}
	} else {
		if (BALLDEBUG) {
			drawBlob(b, BLACK);
			printBlob(b);
			cout << "Screening for ratios " << ratio << endl;
		}
		return BAD_VALUE;
	}
	if (ratio < 1.0) {
		int offRat = ROUND2((1.0f - ratio) * RATIO_TO_INT);
		r -= offRat;
	} else {
		int offRat = ROUND2((1.0f - 1.0f/ratio) * RATIO_TO_INT);
		r -= offRat;
	}
	if (w * h > SMALLBALL) {
		// now make some scans through the blob - horizontal, vertical, diagonal
		int pix;
		int goodPix = 0, badPix = 0;
		if (y + h > IMAGE_HEIGHT - IMAGE_EDGE || x == 0 || (x + w) >
		IMAGE_WIDTH - 2 || y == 0) {
		} else {
			// we're in the screen
			int d = ROUND2(static_cast<float>(std::max(w, h)) /
						   CORNER_CHUNK_DIV);
			int d3 = min(w, h);
			for (int i = 0; i < d3; i++) {
				pix = thresh->thresholded[y+i][x+i];
				if (i < d || (i > d3 - d)) {
					if (pix == ORANGE || pix == ORANGERED) {
						//drawPoint(x+i, y+i, BLACK);
						badPix++;
					}
					else
						goodPix++;
				} else {
					if (pix == ORANGE || pix == ORANGERED || pix == ORANGEYELLOW)
						goodPix++;
					else if (pix != GREY) {
						badPix++;
						//drawPoint(x+i, y+i, PINK);
					}
				}
				pix = thresh->thresholded[y+i][x+w-i];
				if (i < d || (i > d3 - d)) {
					if (pix == ORANGE || pix == ORANGERED) {
						//drawPoint(x+w-i, y+i, BLACK);
						badPix++;
					}
					else
						goodPix++;
				} else if (pix == ORANGE || pix == ORANGERED ||
						pix == ORANGEYELLOW)
					goodPix++;
				else if (pix != GREY) {
					badPix++;
					//drawPoint(x+w-i, y+i, BLACK);
				}
			}
			//cout << "here" << endl;
			for (int i = 0; i < h; i++) {
				pix = thresh->thresholded[y+i][x + w/2];
				//drawPoint(x + w/2, y+i, BLACK);
				if (pix == ORANGE || pix == ORANGERED || pix == ORANGEYELLOW) {
					goodPix++;
				} else if (pix != GREY)
					badPix++;
			}
		}
		for (int i = 0; i < w; i++) {
			pix = thresh->thresholded[y+h/2][x + i];
			//drawPoint(x+i, y+h/2, BLACK);
			if (pix == ORANGE || pix == ORANGERED || pix == ORANGEYELLOW) {
				goodPix++;
			} else if (pix != GREY)
				badPix++;
		}
		if (BALLDEBUG)
			cout << "Roundness: Good " << goodPix << " " << badPix << endl;
		// if more than 20% or so of our pixels tested are bad, then we toss it out
		if (goodPix < badPix * 5) {
			if (BALLDEBUG)
				cout << "Screening for bad roundness" << endl;
			return BAD_VALUE;
		}
	}
	return 0;
}

/*	Check the information surrounding the ball and look to see if it might be a
 * false ball.	Since our main candidate for false balls is the red uniform, the
 * main thing we worry about is a preponderance of red.
 *
 * @param b	   our ball candidate
 * @return	   true if the surround looks bad, false if its ok
 */

bool Ball::badSurround(Blob b) {
	// basically check around the blob and see if it is ok - ideally we'd have
	// some green, worrisome would be lots of RED
	static const int SURROUND = 12;

	const float GREEN_PERCENT = 0.1f;

	int x = b.getLeftTopX();
	int y = b.getLeftTopY();
	int w = b.width();
	int h = b.height();
	int surround = min(SURROUND, w/2);
	int greens = 0, orange = 0, red = 0, borange = 0, pix, realred = 0,
			yellows = 0;

	// first collect information on the ball itself
	for (int i = -1; i < w+1; i++) {
		for (int j = -1; j < h+1; j++) {
			if (x + i > -1 && x + i < IMAGE_WIDTH && y + j > -1 &&
					y + j < IMAGE_HEIGHT) {
				pix = thresh->thresholded[y + j][x + i];
				if (pix == ORANGE)
					borange++;
			}
		}
	}

	// now collect information on the area surrounding the ball
	x = max(0, x - surround);
	y = max(0, y - surround);
	w = w + surround * 2;
	h = h + surround * 2;
	for (int i = 0; i < w && x + i < IMAGE_WIDTH; i++) {
		for (int j = 0; j < h && y + j < IMAGE_HEIGHT; j++) {
			pix = thresh->thresholded[y + j][x + i];
			if (pix == ORANGE || pix == ORANGEYELLOW)
				orange++;
			else if (pix == RED)
				realred++;
			else if (pix == ORANGERED)
				red++;
			else if (pix == GREEN)
				greens++;
			else if (pix == YELLOW && j < surround)
				yellows++;
		}
	}
	if (BALLDEBUG) {
		cout << "Surround information " << red << " " << realred << " "
				<< orange << " " << borange << " " << greens << " "
				<< yellows << endl;
	}
	if (realred > borange) {
		if (BALLDEBUG) {
			cout << "Too much real red" << endl;
		}
		return true;
	}
	if (realred > greens && b.width() * b.height() < 500) {
		if (BALLDEBUG) {
			cout << "Too much real red versus green" << endl;
		}
		return true;
	}
	if (realred > borange && realred > orange) {
		if (BALLDEBUG) {
			cout << "Too much real red vs borange" << endl;
		}
		return true;
	}
	if (orange - borange > borange * 0.3 && orange - borange > 10) {
		if (BALLDEBUG) {
			cout << "Too much orange outside of the ball" << endl;
		}
		// We can run into this problem with reflections - let's see if
		// we're up against a post or something
		if (yellows > w * 3) {
			if (BALLDEBUG) {
				cout << "But lots of yellow, doing nothing " << endl;
			}
			return false;
		} else {
			return true;
		}
	}
	if ((red > orange) &&
			(static_cast<float>(greens) <
					(static_cast<float>(w * h) * GREEN_PERCENT))) {
		if (BALLDEBUG) {
			cout << "Too much real orangered without enough green" << endl;
		}
		return true;
	}
	if (red > orange)  {
		if (BALLDEBUG) {
			cout << "Too much real red - doing more checking" << endl;
		}
		x = b.getLeftTopX();
		y = b.getLeftBottomY();
		if ((x < 1 || x + w > IMAGE_WIDTH - 2) && y	 > IMAGE_HEIGHT - 2) {
			if (BALLDEBUG) {
				cout << "Dangerous corner location detected " << x << " " << y <<  endl;
			}
			return true;
		}
		return roundness(b) == BAD_VALUE;
	}
	return false;
}

/* Once we have determined a ball is a blob we want to set it up for
   the rest of the world (localization, behavior, etc.).
   @param w         ball width
   @param h         ball height
   @param thisBall  the ball
   @param e         pixEstimate to ball
 */

void Ball::setBallInfo(int w, int h, VisualBall *thisBall, estimate e) {

	const float radDiv = 2.0f;
	// x, y, width, and height. Not up for debate.
	thisBall->setX(topBlob->getLeftTopX());
	thisBall->setY(topBlob->getLeftTopY());

	thisBall->setWidth( static_cast<float>(w) );
	thisBall->setHeight( static_cast<float>(h) );
	thisBall->setRadius( std::max(static_cast<float>(w)/radDiv,
								  static_cast<float>(h)/radDiv ) );
	int amount = h / 2;
	if (w > h)
		amount = w / 2;

	if (occlusion == LEFTOCCLUSION) {
		thisBall->setCenterX(topBlob->getRightTopX() - amount);
		thisBall->setX(topBlob->getRightTopX() - amount * 2);
	} else {
		thisBall->setCenterX(topBlob->getLeftTopX() + amount);
	}
	if (occlusion != TOPOCCLUSION) {
		thisBall->setCenterY(topBlob->getLeftTopY() + amount);
	} else {
		thisBall->setCenterY(topBlob->getLeftBottomY() - amount);
	}
	thisBall->setConfidence(SURE);
	thisBall->findAngles();
	if (occlusion == NOOCCLUSION) {
		thisBall->setFocalDistanceFromRadius();
		//trust pixest to within 300 cm
		if (e.dist <= 300) {
			thisBall->setDistanceEst(e);
		}
		else {
			thisBall->setDistanceEst(vision->pose->sizeBasedEstimate(thisBall->getCenterX(),
																	 thisBall->getCenterY(),
																	 ORANGE_BALL_RADIUS,
																	 thisBall->getRadius(),
																	 ORANGE_BALL_RADIUS));
		}
	} else {
		// user our super swell updated pix estimate to do the distance
		thisBall->setDistanceEst(e);
		if (BALLDISTDEBUG) {
			thisBall->setFocalDistanceFromRadius();
		}
	}
	/*cout<<"pixest "<<e.dist<<"size "<<vision->pose->sizeBasedEstimate(thisBall->getCenterX(),
																	  thisBall->getCenterY(),
																	  ORANGE_BALL_RADIUS,
																	  thisBall->getRadius(),
																	  ORANGE_BALL_RADIUS).dist<<endl;*/
}

/* Misc. routines
 */

/* When we process blobs we start them with BAD_VALUE such that we can easily
 * tell if whatever processing we did worked out.  Here we make that check.
 * @param b	   the blob we worked on.
 * @return	   true when the processing worked, false otherwise
 */
bool Ball::blobOk(Blob b) {
	if (b.getLeftTopX() > BAD_VALUE && b.getLeftBottomX() > BAD_VALUE &&
			b.width() > 2)
		return true;
	return false;
}


/* Print debugging information for a blob.
 * @param b	   the blob
 */
void Ball::printBlob(Blob b) {
#if defined OFFLINE
	cout << "Outputting blob" << endl;
	cout << b.getLeftTopX() << " " << b.getLeftTopY() << " " << b.getRightTopX()
				 << " " << b.getRightTopY() << endl;
	cout << b.getLeftBottomX() << " " << b.getLeftBottomY() << " "
			<< b.getRightBottomX() << " " << b.getRightBottomY() << endl;
#endif
}

/* Prints a bunch of ball information about the best ball candidate (or any one).
 * @param b	   the candidate ball
 * @param c	   how confident we are its a ball
 * @param p	   how many occlusions
 * @param o	   what the occlusions are if any
 * @param bg   where around the ball there is green
 */
void Ball::printBall(Blob b, int c, float p, int o) {
#ifdef OFFLINE
	if (BALLDEBUG) {
		cout << "Ball info: " << b.getLeftTopX() << " " << b.getLeftTopY()
					 << " " << b.width() << " " << b.height() << endl;
		cout << "Confidence: " << c << " Orange Percent: " << p
				<< " Occlusions: ";
		if (o == NOOCCLUSION) cout <<  "none";
		if (o % LEFTOCCLUSION == 0) cout << "left ";
		if (o % RIGHTOCCLUSION == 0) cout << "right ";
		if (o % TOPOCCLUSION == 0) cout << "top ";
		if (o % BOTTOMOCCLUSION == 0) cout << "bottom ";
		cout << endl;
	}
#endif
}

/* Debugging method used to show where things were processed on the image.
 * Paints a verticle stripe corresponding to a "run" of color.
 *
 * @param x		x coord
 * @param y		y coord
 * @param h		height
 * @param c		the color to paint
 */
void Ball::paintRun(int x, int y, int h, int c){
	vision->drawLine(x,y+1,x,y+h+1,c);
}

/*	More or less the same as the previous method, but with different parameters.
 * @param run	  a run of color
 * @param c		  the color to paint
 */
void Ball::drawRun(const run& run, int c) {
	vision->drawLine(run.x,run.y+1,run.x,run.y+run.h+1,c);
}

/*	Draws a "+" on the screen at the specified location with the specified color.
 * @param x		x coord
 * @param y		y coord
 * @param c		the color to paint
 */
void Ball::drawPoint(int x, int y, int c) {
#ifdef OFFLINE
	thresh->drawPoint(x, y, c);
#endif
}

/*	Draws the outline of a rectangle in the specified color.
 * @param b	   the rectangle
 * @param c	   the color to paint
 */
void Ball::drawRect(int x, int y, int w, int h, int c) {
#ifdef OFFLINE
	thresh->drawRect(x, y, w, h, c);
#endif
}

/*	Draws the outline of a blob in the specified color.
 * @param b	   the blob
 * @param c	   the color to paint
 */
void Ball::drawBlob(Blob b, int c) {
#ifdef OFFLINE
	thresh->drawLine(b.getLeftTopX(), b.getLeftTopY(),
					 b.getRightTopX(), b.getRightTopY(),
					 c);
	thresh->drawLine(b.getLeftTopX(), b.getLeftTopY(),
					 b.getLeftBottomX(), b.getLeftBottomY(),
					 c);
	thresh->drawLine(b.getLeftBottomX(), b.getLeftBottomY(),
					 b.getRightBottomX(), b.getRightBottomY(),
					 c);
    thresh->drawLine(b.getRightTopX(), b.getRightTopY(),
					 b.getRightBottomX(), b.getRightBottomY(),
					 c);
#endif
}

/* Draws a line on the screen of the specified color.
 * @param x	   x value of point 1
 * @param y	   y value of point 1
 * @param x1   x value of point 2
 * @param y1   y value of point 2
 * @param c	   the color to paint the line.
 */
void Ball::drawLine(int x, int y, int x1, int y1, int c) {
#ifdef OFFLINE
	thresh->drawLine(x, y, x1, y1, c);
#endif
}

