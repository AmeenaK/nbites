#include "ConcreteCorner.h"
using namespace std;

ConcreteCorner::ConcreteCorner(const float _fieldX, const float _fieldY,
                               const cornerID _id)
    : ConcreteLandmark(_fieldX, _fieldY), id(_id) {
    cornerType = inferCornerType(_id);
	if (cornerType == T){
		assignTCornerLines();
	}
}
ConcreteCorner::ConcreteCorner(const float _fieldX, const float _fieldY,
                               const ConcreteLine& _l1, const ConcreteLine& _l2,
                               const cornerID _id)
    : ConcreteLandmark(_fieldX, _fieldY), id(_id) ,
	  line1(&_l1), line2(&_l2), lines()
{
    cornerType = inferCornerType(_id);
	lines.push_back(line1);
	lines.push_back(line2);
	if (cornerType == T){
		assignTCornerLines();
	}
}

// Copy constructor - just pairwise copy the elements
ConcreteCorner::ConcreteCorner(const ConcreteCorner& other)
    : ConcreteLandmark(other), id(other.id), cornerType(other.cornerType) {
}

// No memory to free
ConcreteCorner::~ConcreteCorner() {}

///////////////////////////////////////////////////////////////////////////////
// Constants for absolute coordinates on the field of the specific corners.
// (0,0) is the lower left corner of the field when BLUE goal is at the
// bottom
///////////////////////////////////////////////////////////////////////////////
const ConcreteCorner& ConcreteCorner::blue_corner_top_l(){
	static const ConcreteCorner corner(FIELD_WHITE_LEFT_SIDELINE_X,
									   FIELD_WHITE_TOP_SIDELINE_Y,
									   ConcreteLine::blue_goal_endline(),
									   ConcreteLine::top_sideline(),
									   BLUE_CORNER_TOP_L);
	return corner;
}

const ConcreteCorner& ConcreteCorner::blue_corner_bottom_l(){
	static const ConcreteCorner corner(FIELD_WHITE_LEFT_SIDELINE_X,
									   FIELD_WHITE_BOTTOM_SIDELINE_Y,
									   ConcreteLine::blue_goal_endline(),
									   ConcreteLine::bottom_sideline(),
									   BLUE_CORNER_BOTTOM_L);
	return corner;
}

const ConcreteCorner& ConcreteCorner::blue_goal_left_t(){
	static const ConcreteCorner corner(BLUE_GOALBOX_LEFT_X,
									   BLUE_GOALBOX_TOP_Y,
									   ConcreteLine::blue_goal_endline(),
									   ConcreteLine::blue_goalbox_left_line(),
									   BLUE_GOAL_LEFT_T);
	return corner;
}

const ConcreteCorner& ConcreteCorner::blue_goal_right_t()
{
	static const ConcreteCorner corner(BLUE_GOALBOX_LEFT_X,
									   BLUE_GOALBOX_BOTTOM_Y,
									   ConcreteLine::blue_goal_endline(),
									   ConcreteLine::blue_goalbox_right_line(),
									   BLUE_GOAL_RIGHT_T);
	return corner;
}

const ConcreteCorner& ConcreteCorner::blue_goal_left_l(){
	static const ConcreteCorner corner(BLUE_GOALBOX_RIGHT_X,
									   BLUE_GOALBOX_TOP_Y,
									   ConcreteLine::blue_goalbox_left_line(),
									   ConcreteLine::blue_goalbox_top_line(),
									   BLUE_GOAL_LEFT_L);
	return corner;
}

const ConcreteCorner& ConcreteCorner::blue_goal_right_l(){
	static const ConcreteCorner corner(BLUE_GOALBOX_RIGHT_X,
									   BLUE_GOALBOX_BOTTOM_Y,
									   ConcreteLine::blue_goalbox_right_line(),
									   ConcreteLine::blue_goalbox_top_line(),
									   BLUE_GOAL_RIGHT_L);
	return corner;
}

const ConcreteCorner& ConcreteCorner::yellow_corner_bottom_l(){
	static const ConcreteCorner corner(FIELD_WHITE_RIGHT_SIDELINE_X,
									   FIELD_WHITE_BOTTOM_SIDELINE_Y,
									   ConcreteLine::yellow_goal_endline(),
									   ConcreteLine::bottom_sideline(),
									   YELLOW_CORNER_BOTTOM_L);
	return corner;
}

const ConcreteCorner& ConcreteCorner::yellow_corner_top_l(){
	static const ConcreteCorner corner(FIELD_WHITE_RIGHT_SIDELINE_X,
									   FIELD_WHITE_TOP_SIDELINE_Y,
									   ConcreteLine::yellow_goal_endline(),
									   ConcreteLine::top_sideline(),
									   YELLOW_CORNER_TOP_L);
	return corner;
}

const ConcreteCorner& ConcreteCorner::yellow_goal_left_t(){
	static const ConcreteCorner corner(YELLOW_GOALBOX_RIGHT_X,
									   YELLOW_GOALBOX_BOTTOM_Y,
									   ConcreteLine::yellow_goal_endline(),
									   ConcreteLine::yellow_goalbox_left_line(),
									   YELLOW_GOAL_LEFT_T);
	return corner;
}

const ConcreteCorner& ConcreteCorner::yellow_goal_right_t(){
	static const ConcreteCorner corner(YELLOW_GOALBOX_RIGHT_X,
									   YELLOW_GOALBOX_TOP_Y,
									   ConcreteLine::yellow_goal_endline(),
									   ConcreteLine::yellow_goalbox_right_line(),
									   YELLOW_GOAL_RIGHT_T);
	return corner;
}

const ConcreteCorner& ConcreteCorner::yellow_goal_left_l(){
	static const ConcreteCorner corner(YELLOW_GOALBOX_LEFT_X,
									   YELLOW_GOALBOX_BOTTOM_Y,
									   ConcreteLine::yellow_goalbox_left_line(),
									   ConcreteLine::yellow_goalbox_top_line(),
									   YELLOW_GOAL_LEFT_L);
	return corner;
}

const ConcreteCorner& ConcreteCorner::yellow_goal_right_l(){
	static const ConcreteCorner corner(YELLOW_GOALBOX_LEFT_X,
									   YELLOW_GOALBOX_TOP_Y,
									   ConcreteLine::yellow_goalbox_right_line(),
									   ConcreteLine::yellow_goalbox_top_line(),
									   YELLOW_GOAL_RIGHT_L);
	return corner;
}

const ConcreteCorner& ConcreteCorner::top_cc(){
	static const ConcreteCorner corner(TOP_CC_X, TOP_CC_Y, TOP_CC);
	return corner;
}

const ConcreteCorner& ConcreteCorner::bottom_cc(){
	static const ConcreteCorner corner(BOTTOM_CC_X, BOTTOM_CC_Y, BOTTOM_CC);
	return corner;
}

// Not added to the concreteCornerList, because this corner is fake and only
// Exists because of a sanity check in FieldLines
const ConcreteCorner& ConcreteCorner::fake_cc()
{
	static const ConcreteCorner corner(CENTER_FIELD_X, CENTER_FIELD_Y, CENTER_CIRCLE);
	return corner;
}

const ConcreteCorner& ConcreteCorner::center_bottom_t(){
	static const ConcreteCorner corner(MIDFIELD_X,
									   FIELD_WHITE_BOTTOM_SIDELINE_Y,
									   ConcreteLine::bottom_sideline(),
									   ConcreteLine::midline(),
									   CENTER_BOTTOM_T);
	return corner;
}

const ConcreteCorner& ConcreteCorner::center_top_t(){
	static const ConcreteCorner corner(MIDFIELD_X,
									   FIELD_WHITE_TOP_SIDELINE_Y,
									   ConcreteLine::top_sideline(),
									   ConcreteLine::midline(),
									   CENTER_TOP_T);
	return corner;
}

const vector <const ConcreteCorner*>& ConcreteCorner::concreteCorners()
{
	const ConcreteCorner* corners_array[NUM_CORNERS] =
		{&blue_corner_top_l(),
		 &blue_corner_bottom_l(),
		 &blue_goal_left_t(),
		 &blue_goal_right_t(),
		 &blue_goal_left_l(),
		 &blue_goal_right_l(),
		 &center_top_t(),
		 &center_bottom_t(),
		 &yellow_corner_bottom_l(),
		 &yellow_corner_top_l(),
		 &yellow_goal_left_t(),
		 &yellow_goal_right_t(),
		 &yellow_goal_left_l(),
		 &yellow_goal_right_l(),
		 &top_cc(),
		 &bottom_cc()
		};
	static const vector<const ConcreteCorner*>
		corners(corners_array,
				corners_array + sizeof(corners_array) / sizeof(corners_array[0]));
	return corners;
}

const vector <const ConcreteCorner*>& ConcreteCorner::lCorners()
{
	static const ConcreteCorner* corners_array[NUM_L_CORNERS] = {
		&blue_corner_top_l(),
		&blue_corner_bottom_l(),
		&blue_goal_left_l(),
		&blue_goal_right_l(),
		&yellow_corner_bottom_l(),
		&yellow_corner_top_l(),
		&yellow_goal_left_l(),
		&yellow_goal_right_l()
	};
	static const vector<const ConcreteCorner*>
		corners(corners_array,
				corners_array + sizeof(corners_array) / sizeof(corners_array[0]));
	return corners;
}

const vector <const ConcreteCorner*>& ConcreteCorner::tCorners()
{
	const ConcreteCorner* corners_array[NUM_T_CORNERS] = {
		&blue_goal_left_t(),
		&blue_goal_right_t(),
		&yellow_goal_left_t(),
		&yellow_goal_right_t(),
		&center_top_t(),
		&center_bottom_t()
	};
	static const vector<const ConcreteCorner*>
		corners(corners_array,
				corners_array + sizeof(corners_array) / sizeof(corners_array[0]));
	return corners;
}

const vector <const ConcreteCorner*>& ConcreteCorner::ccCorners()
{
	const ConcreteCorner* corners_array[NUM_CC_CORNERS] = {
		&top_cc(),
		&bottom_cc()
	};
	static const vector<const ConcreteCorner*>
		corners(corners_array,
				corners_array + sizeof(corners_array) / sizeof(corners_array[0]));
	return corners;
}

const vector <const ConcreteCorner*>& ConcreteCorner::blueGoalCorners()
{

	const ConcreteCorner* corners_array[
		NUM_BLUE_GOAL_CORNERS] = {
		&blue_goal_left_l(),
		&blue_goal_right_l(),
		&blue_goal_left_t(),
		&blue_goal_right_t()
	};
	static const vector<const ConcreteCorner*>
		corners(corners_array,
				corners_array + sizeof(corners_array) / sizeof(corners_array[0]));
	return corners;
}

const vector <const ConcreteCorner*>& ConcreteCorner::yellowGoalCorners()
{


	const ConcreteCorner* corners_array[
		NUM_YELLOW_GOAL_CORNERS] = {
		&yellow_goal_left_l(),
		&yellow_goal_right_l(),
		&yellow_goal_left_t(),
		&yellow_goal_right_t()
	};
	static const vector<const ConcreteCorner*>
		corners(corners_array,
				corners_array + sizeof(corners_array) / sizeof(corners_array[0]));
	return corners;
}

const vector <const ConcreteCorner*>& ConcreteCorner::blueGoalTCorners()
{
	const ConcreteCorner* corners_array[
		NUM_BLUE_GOAL_T_CORNERS] =
		{
			&blue_goal_left_t(),
			&blue_goal_right_t()
		};
	static const vector<const ConcreteCorner*>
		corners(corners_array,
				corners_array + sizeof(corners_array) / sizeof(corners_array[0]));
	return corners;
}

const vector <const ConcreteCorner*>& ConcreteCorner::yellowGoalTCorners()
{
	const ConcreteCorner* corners_array[
		NUM_YELLOW_GOAL_T_CORNERS] =
		{
			&yellow_goal_left_t(),
			&yellow_goal_right_t()
		};
	static const vector<const ConcreteCorner*>
		corners(corners_array,
				corners_array + sizeof(corners_array) / sizeof(corners_array[0]));
	return corners;
}


const string ConcreteCorner::getShapeString(shape s) {
    switch (s) {
    case INNER_L:
        return "Inner L";
    case OUTER_L:
        return "Outer L";
    case T:
        return "T";
    case CIRCLE:
        return "Circle";
    case UNKNOWN:
        return "Unknown";
    default:
        throw "Error, invalid shape";
    }

}

const string ConcreteCorner::toString() const {
    return cornerIDToString(id);
}
const string ConcreteCorner::cornerIDToString(const cornerID _id) {
    switch (_id) {
        //Blue ones
    case BLUE_CORNER_TOP_L:
        return "Blue Corner Top L";
    case BLUE_CORNER_BOTTOM_L:
        return "Blue Corner Bottom L";
    case BLUE_GOAL_LEFT_T:
        return "Blue Goal Left T";
    case BLUE_GOAL_RIGHT_T:
        return "Blue Goal Right T";
    case BLUE_GOAL_LEFT_L:
        return "Blue Goal Left L";
    case BLUE_GOAL_RIGHT_L:
        return "Blue Goal Right L";

        // Yellow ones
    case YELLOW_CORNER_BOTTOM_L:
        return "Yellow Corner Bottom L";
    case YELLOW_CORNER_TOP_L:
        return "Yellow Corner Top L";
    case YELLOW_GOAL_RIGHT_T:
        return "Yellow Goal Right T";
    case YELLOW_GOAL_LEFT_T:
        return "Yellow Goal Left T";
    case YELLOW_GOAL_RIGHT_L:
        return "Yellow Goal Right L";
    case YELLOW_GOAL_LEFT_L:
        return "Yellow Goal Left L";

        // Center ones
    case CENTER_BOTTOM_T:
        return "Center Bottom T";
    case CENTER_TOP_T:
        return "Center Top T";
    case CENTER_CIRCLE:
        return "Center Circle Corner";

        // Abstract corners
    case L_INNER_CORNER:
        return "L Inner Corner";
    case L_OUTER_CORNER:
        return "L Outer Corner";
    case CORNER_INNER_L:
        return "Corner Inner L";
    case T_CORNER:
        return "T Corner";
    case BLUE_GOAL_T:
        return "Blue Goal T";
    case YELLOW_GOAL_T:
        return "Yellow Goal T";
    case CENTER_T:
        return "Center T";
    case BLUE_GOAL_RIGHT_L_OR_YELLOW_GOAL_LEFT_L:
        return "Blue Goal Right L Or Yellow Goal Left L";
    case BLUE_GOAL_LEFT_L_OR_YELLOW_GOAL_RIGHT_L:
        return "Blue Goal Left L Or Yellow Goal Right L";
    case BLUE_CORNER_TOP_L_OR_YELLOW_CORNER_BOTTOM_L:
        return "Blue Corner Top L Or Yellow Corner Bottom L";
    case BLUE_CORNER_BOTTOM_L_OR_YELLOW_CORNER_TOP_L:
        return "Blue Corner Bottom L Or Yellow Corner Top L";
    case GOAL_BOX_INNER_L:
        return "Goal Box Inner L";
    case BLUE_GOAL_OUTER_L:
        return "Blue Goal Outer L";
    case YELLOW_GOAL_OUTER_L:
        return "Yellow Goal Outer L";

    case CORNER_NO_IDEA_ID:
        return "Unknown Corner";

    case TOP_CC:
        return "Top Center Circle";
    case BOTTOM_CC:
        return "Bottom Center Circle";

    default:
         return "Invalid Corner Id";
    }
}

const shape ConcreteCorner::inferCornerType(const cornerID id) {
    switch (id) {
    case L_INNER_CORNER:
    case L_OUTER_CORNER:
    case BLUE_GOAL_RIGHT_L_OR_YELLOW_GOAL_LEFT_L:
    case BLUE_GOAL_LEFT_L_OR_YELLOW_GOAL_RIGHT_L:
    case BLUE_CORNER_TOP_L_OR_YELLOW_CORNER_BOTTOM_L:
    case BLUE_CORNER_BOTTOM_L_OR_YELLOW_CORNER_TOP_L:
    case CORNER_INNER_L:
    case GOAL_BOX_INNER_L:
    case BLUE_GOAL_OUTER_L:
    case YELLOW_GOAL_OUTER_L:
    case BLUE_CORNER_TOP_L:
    case BLUE_CORNER_BOTTOM_L:
    case BLUE_GOAL_LEFT_L:
    case BLUE_GOAL_RIGHT_L:
    case YELLOW_CORNER_TOP_L:
    case YELLOW_CORNER_BOTTOM_L:
    case YELLOW_GOAL_LEFT_L:
    case YELLOW_GOAL_RIGHT_L:
        return INNER_L;

    case T_CORNER:
    case BLUE_GOAL_T:
    case YELLOW_GOAL_T:
    case YELLOW_GOAL_LEFT_T:
    case YELLOW_GOAL_RIGHT_T:
    case CENTER_TOP_T:
    case CENTER_BOTTOM_T:
    case BLUE_GOAL_LEFT_T:
    case BLUE_GOAL_RIGHT_T:
    case CENTER_T:
        return T;
    case CENTER_CIRCLE:
	case TOP_CC:
	case BOTTOM_CC:
        return CIRCLE;
    case CORNER_NO_IDEA_ID:
    default:
        return UNKNOWN;
    }
}
/**
 * Checks all the tBar lines to see if either of the lines in this corner
 * are a bar. Relies on the fact that no line is both a tStem and a tBar.
 */
void ConcreteCorner::assignTCornerLines()
{
	vector<const ConcreteLine*>::const_iterator i = ConcreteLine::tBarLines().begin();
	while ( i != ConcreteLine::tBarLines().end() ) {
		if (**i == *line1) {
			tBar = line1;
			tStem = line2;
			break;
		} else if (**i == *line2) {
			tBar = line2;
			tStem = line1;
			break;
		}
		i++;
	}

}


/**
 * Given a shape, returns the concrete corners that this shaped
 * corner could possibly be.
 */
const vector <const ConcreteCorner*>& ConcreteCorner::
getPossibleCorners(shape corner_type) {
    vector <const ConcreteCorner*> possible;
    switch (corner_type) {
    case INNER_L:
        // intentional fall through
    case OUTER_L:
        return lCorners();
    case T:
        return tCorners();
    case CIRCLE:
        return ccCorners();
    default:
        // Should never be reached
        throw -1;
    }
}

bool ConcreteCorner::isLineInCorner(const ConcreteLine* line) const
{
	return (line == line1) || (line == line2);
}

