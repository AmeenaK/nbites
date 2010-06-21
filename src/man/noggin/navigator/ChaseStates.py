from . import NavHelper as helper
from . import WalkHelper as walker
from . import NavTransitions as navTrans
from . import NavConstants as constants
from . import BrunswickSpeeds as speeds
from man.noggin.util import MyMath
from man.noggin.typeDefs.Location import RobotLocation
from man.noggin import NogginConstants
from man.noggin.playbook.PBConstants import GOALIE
from math import fabs

DEBUG = False

def walkSpinToBall(nav):
    ball = nav.brain.ball
    nav.dest = ball
    nav.dest.h = ball.heading

    if navTrans.atDestinationCloserAndFacing(nav.brain.my, nav.dest, ball.bearing):
        return nav.goNow('stop')

    # Set our walk towards the ball
    walkX, walkY, walkTheta = \
           walker.getWalkSpinParam(nav.brain.my, nav.dest)

    helper.setSpeed(nav, walkX, walkY, walkTheta)

    if not nav.brain.play.isRole(GOALIE):
        if navTrans.shouldNotGoInBox(ball):
            return nav.goLater('ballInMyBox')
        elif navTrans.shouldChaseAroundBox(nav.brain.my, ball):
            return nav.goLater('chaseAroundBox')
        elif navTrans.shouldAvoidObstacleDuringApproachBall(nav):
            return nav.goLater('avoidObstacle')

    return nav.stay()

# TODO: clean up!!
def chaseAroundBox(nav):
    # does not work if we are in corner and ball is not- we'll run through box
    # would be nice to force spin towards ball at dest

    if nav.firstFrame():
        # reset dest to new RobotLocation to avoid problems w/dist calculations
        nav.dest = RobotLocation()
        nav.shouldChaseAroundBox = 0

    ball = nav.brain.ball
    my = nav.brain.my

    if not navTrans.shouldChaseAroundBox(my, ball):
        nav.shouldntChaseAroundBox += 1
    else:
        nav.shouldntChaseAroundBox = 0

    if nav.shouldntChaseAroundBox > constants.STOP_CHASING_AROUND_BOX:
        return nav.goNow('walkSpinToBall')

    elif navTrans.shouldAvoidObstacleDuringApproachBall(nav):
        return nav.goNow('avoidObstacle')

    if my.x > NogginConstants.MY_GOALBOX_RIGHT_X:
        # go to corner nearest ball
        if ball.y > NogginConstants.MY_GOALBOX_TOP_Y:
            nav.dest.x = (NogginConstants.MY_GOALBOX_RIGHT_X +
                          constants.GOALBOX_OFFSET)
            nav.dest.y = (NogginConstants.MY_GOALBOX_TOP_Y +
                          constants.GOALBOX_OFFSET)
            nav.dest.h = my.headingTo(nav.dest)

        elif ball.y < NogginConstants.MY_GOALBOX_BOTTOM_Y:
            nav.dest.x = (NogginConstants.MY_GOALBOX_RIGHT_X +
                          constants.GOALBOX_OFFSET)
            nav.dest.y = (NogginConstants.MY_GOALBOX_BOTTOM_Y -
                          constants.GOALBOX_OFFSET)
            nav.dest.h = my.headingTo(nav.dest)

    if my.x < NogginConstants.MY_GOALBOX_RIGHT_X:
        # go to corner nearest ball
        if my.y > NogginConstants.MY_GOALBOX_TOP_Y:
            nav.dest.x = (NogginConstants.MY_GOALBOX_RIGHT_X +
                          constants.GOALBOX_OFFSET)
            nav.dest.y = (NogginConstants.MY_GOALBOX_TOP_Y +
                          constants.GOALBOX_OFFSET)
            nav.dest.h = my.headingTo(nav.dest)

        if my.y < NogginConstants.MY_GOALBOX_BOTTOM_Y:
            nav.dest.x = (NogginConstants.MY_GOALBOX_RIGHT_X +
                          constants.GOALBOX_OFFSET)
            nav.dest.y = (NogginConstants.MY_GOALBOX_BOTTOM_Y -
                          constants.GOALBOX_OFFSET)
            nav.dest.h = my.headingTo(nav.dest)

    walkX, walkY, walkTheta = walker.getWalkSpinParam(my, nav.dest)
    helper.setSpeed(nav, walkX, walkY, walkTheta)

    return nav.stay()

def ballInMyBox(nav):
    if nav.firstFrame():
        nav.brain.tracker.activeLoc()

    ball = nav.brain.ball

    if fabs(ball.bearing) > constants.BALL_APPROACH_BEARING_THRESH:
        nav.setWalk(0, 0,
                    constants.BALL_SPIN_SPEED * MyMath.sign(ball.bearing) )

    elif fabs(ball.bearing) < constants.BALL_APPROACH_BEARING_OFF_THRESH :
        nav.stopWalking()

    if not nav.ball.inMyGoalBox():
        return nav.goLater('chase')

    return nav.stay()

# Values for controlling the strafing
PFK_MAX_Y_SPEED = speeds.MAX_Y_SPEED
PFK_MIN_Y_SPEED = speeds.MIN_Y_SPEED
PFK_MAX_X_SPEED = speeds.MAX_X_SPEED
PFK_MIN_X_SPEED = -speeds.MIN_X_SPEED
PFK_MIN_Y_MAGNITUDE = speeds.MIN_Y_MAGNITUDE
PFK_MIN_X_MAGNITUDE = speeds.MIN_X_MAGNITUDE
PFK_X_GAIN = 0.12
PFK_Y_GAIN = 0.6
PFK_X_SAFE_DISTANCE = 12.

PFK_CLOSE_ENOUGH_THETA = 11
PFK_CLOSE_ENOUGH_XY = 3.0
PFK_BALL_CLOSE_ENOUGH = 30

LEFT_KICK_OFFSET = 5.0  # taken from LEFT_FOOT_L_Y and R_Y, see: KickingConstants

def positionForKick(nav):
    ball = nav.brain.ball

    if nav.firstFrame() or ball.dist > PFK_BALL_CLOSE_ENOUGH:
        nav.doneTheta = False
        nav.doneX = False
        nav.doneY = False

    if (nav.doneX and nav.doneY and nav.doneTheta):
        return nav.goNow('stop')

    helper.setSlowSpeed(nav,positionForKickX(nav),positionForKickY(nav),
                        positionForKickTheta(nav))

    return nav.stay()

def positionForKickX(nav):
    ball = nav.brain.ball

    targetX = ball.relX - PFK_X_SAFE_DISTANCE
    if fabs(targetX) < PFK_CLOSE_ENOUGH_XY:
        sX = 0.0
        nav.doneX = True
    else:
        sX = MyMath.clip(targetX * PFK_X_GAIN,
                         PFK_MIN_X_SPEED,
                         PFK_MAX_X_SPEED)
        sX = max(PFK_MIN_X_MAGNITUDE,sX) * MyMath.sign(sX)

    return sX

def positionForKickY(nav):
    ball = nav.brain.ball
    target_y = ball.relY - LEFT_KICK_OFFSET

    if fabs(target_y) < PFK_CLOSE_ENOUGH_XY:
        nav.doneY = True
        sY = 0.0
    else:
        sY = MyMath.clip(target_y * PFK_Y_GAIN,
                         PFK_MIN_Y_SPEED,
                         PFK_MAX_Y_SPEED)
        sY = max(PFK_MIN_Y_MAGNITUDE,sY) * MyMath.sign(sY)

    return sY

def positionForKickTheta(nav):
    ball = nav.brain.ball
    hDiff = MyMath.sub180Angle(ball.bearing)

    if fabs(hDiff) < PFK_CLOSE_ENOUGH_THETA:
        sTheta = 0.0
        nav.doneTheta = True
    else:
        sTheta = MyMath.sign(hDiff) * constants.GOTO_SPIN_SPEED * \
                 walker.getRotScale(hDiff)
        sTheta = MyMath.clip(sTheta,
                             constants.OMNI_MIN_SPIN_SPEED,
                             constants.OMNI_MAX_SPIN_SPEED)

    return sTheta

def dribble(nav):
    ball = nav.brain.ball
    nav.dest = ball
    nav.dest.h = ball.heading

    # Set our walk towards the ball
    walkX, walkY, walkTheta = \
           walker.getWalkSpinParam(nav.brain.my, nav.dest)

    helper.setDribbleSpeed(nav, walkX, walkY, walkTheta)

    if not nav.brain.play.isRole(GOALIE):
        if navTrans.shouldNotGoInBox(ball):
            return nav.goLater('ballInMyBox')
        elif navTrans.shouldChaseAroundBox(nav.brain.my, ball):
            return nav.goLater('chaseAroundBox')

    return nav.stay()
