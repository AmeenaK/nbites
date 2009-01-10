/**
 * Observation.cpp - The landmark observation class. Here we house all those
 * things needed to describe a single landmark sighting.  Observations can be of
 * field objects, corners (line intersections), lines, and possibly other
 * things in the future (i.e. carpet edges)
 *
 * @author Tucker Hermans
 */

#include "Observation.h"

/**
 * @param fo FieldObject that was seen and reported.
 */
Observation::Observation(FieldObjects &_object)
{
    // We aren't a line
    line_truth = false;
    visDist = _object.getDist();
    visBearing = _object.getBearing();
    // sigma_d = _object.getDistanceSD();
    // sigma_b = _object.getBearingSD();
    sigma_d = visDist * 4.0f;
    sigma_b = visBearing * 4.0f;

    id = _object.getID();

    // Figure out which possible landmarks we have...
    // This should be cleaner like in corners, once field objects is in line...
    if (_object.getCertainty() == SURE) {
        PointLandmark objectLandmark;
        if ( id == BLUE_GOAL_LEFT_POST) {
            objectLandmark.x = ConcreteFieldObject::
                blue_goal_left_post.getFieldX();
            objectLandmark.y = ConcreteFieldObject::
                blue_goal_left_post.getFieldY();
        } else if( id == BLUE_GOAL_RIGHT_POST) {
            objectLandmark.x = ConcreteFieldObject::
                blue_goal_right_post.getFieldX();
            objectLandmark.y = ConcreteFieldObject::
                blue_goal_right_post.getFieldY();
        } else if ( id == YELLOW_GOAL_LEFT_POST) {
            objectLandmark.x = ConcreteFieldObject::
                yellow_goal_left_post.getFieldX();
            objectLandmark.y = ConcreteFieldObject::
                yellow_goal_left_post.getFieldY();
        } else if( id == YELLOW_GOAL_RIGHT_POST) {
            objectLandmark.x = ConcreteFieldObject::
                yellow_goal_right_post.getFieldX();
            objectLandmark.y = ConcreteFieldObject::
                yellow_goal_right_post.getFieldY();
        }
        pointPossibilities.push_back(objectLandmark);
        Observation::numPossibilities = 1;
        return;
    }

    list <const ConcreteFieldObject *> objList;
    if ( id == BLUE_GOAL_LEFT_POST ||
         id == BLUE_GOAL_RIGHT_POST) {
            objList = ConcreteFieldObject::blueGoalPosts;
    } else {
            objList = ConcreteFieldObject::yellowGoalPosts;
    }
    // Initialize to 0 possibilities
    numPossibilities = 0;

    list <const ConcreteFieldObject *>::iterator theIterator;
    //list <const ConcreteFieldObject *> objList = _object.getPossibleFieldObjects();
    for( theIterator = objList.begin(); theIterator != objList.end();
         ++theIterator) {
        PointLandmark objectLandmark((**theIterator).getFieldX(),
                                     (**theIterator).getFieldY());
        pointPossibilities.push_back(objectLandmark);
        ++numPossibilities;
    }
}

/**
 * @param c Corner that was seen and reported.
 */
Observation::Observation(VisualCorner &_corner)
{
    // We aren't a line
    line_truth = false;

    // Get basic vision information
    visDist = _corner.getDistance();
    visBearing = _corner.getBearing();
    //sigma_d = _corner.getDistanceSD();
    //sigma_b = _corner.getBearingSD();
    sigma_d = visDist * 4.0f;
    sigma_b = visBearing * 4.0f;
    //id = _corner.getID();

    // Build our possibilitiy list
    numPossibilities= 0;

    list <const ConcreteCorner *>::iterator theIterator;
    list <const ConcreteCorner *> cornerList = _corner.getPossibleCorners();
    for( theIterator = cornerList.begin(); theIterator != cornerList.end();
         ++theIterator) {
        PointLandmark cornerLandmark((**theIterator).getFieldX(),
                                     (**theIterator).getFieldY());
        pointPossibilities.push_back(cornerLandmark);
        ++numPossibilities;
    }
}

/**
 * @param l Line that was seen and reported.
 */
Observation::Observation(VisualLine &_line)
{
    // We're a line
    line_truth = true;

    // Get basic vision information
    // visDist = _line.getDistance();
    // visBearing = _line.getBearing();
    // sigma_d = _line.getDistanceSD();
    // sigma_b = _line.getBearingSD();
    visDist = 100;
    visBearing = 200;
    sigma_d = visDist * 4.0f;
    sigma_b = visBearing * 4.0f;
    //id = _line.getID();

    // Build our possibilitiy list
    Observation::numPossibilities = 0;

    list <const ConcreteLine *>::iterator theIterator;
    list <const ConcreteLine *> lineList = _line.getPossibleLines();
    for( theIterator = lineList.begin(); theIterator != lineList.end();
         ++theIterator) {
        LineLandmark addLine((**theIterator).getFieldX1(),
                             (**theIterator).getFieldY1(),
                             (**theIterator).getFieldX1(),
                             (**theIterator).getFieldY1());
        linePossibilities.push_back(addLine);
        ++Observation::numPossibilities;
    }

}

Observation::Observation(const ConcreteCorner &_corner)
{
    numPossibilities = 1;
    PointLandmark cornerLandmark(_corner.getFieldX(),_corner.getFieldY());
    pointPossibilities.push_back(cornerLandmark);
}

/**
 * Deconstructor for our observation
 */
Observation::~Observation() {}

// Simple class constructors

PointLandmark::PointLandmark(float _x, float _y): x(_x), y(_y){}

PointLandmark::PointLandmark(){}

LineLandmark::LineLandmark(float _x1, float _y1, float _x2, float _y2):
    x1(_x1), y1(_y2), x2(_x2), y2(_y2){}

LineLandmark::LineLandmark(){}

