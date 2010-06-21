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

def crossoverTowardsBall(nav):
    CROSSOVER_DIST = 40         # Cross over for 40cm

    brain = nav.brain
    my = brain.my
    ball = nav.brain.ball


    # Get location 40cm away in line towards the ball
    destH = my.headingTo(ball)
    distToBall = my.distTo(ball)

    relDestX = ball.relX * CROSSOVER_DIST / distToBall
    relDestY = ball.relY * CROSSOVER_DIST / distToBall

    nav.dest = RobotLocation(relDestX + my.x,
                             relDestY + my.y,
                             destH)

    if (navTrans.atDestinationCloserAndFacing(nav.brain.my,
                                              nav.dest,
                                              ball.bearing) or
        (abs(ball.bearing) < 20)
        and ball.on):
        return nav.goNow('walkSpinToBall')

    if not nav.brain.play.isRole(GOALIE):
        if navTrans.shouldNotGoInBox(ball):
            return nav.goLater('ballInMyBox')
        elif navTrans.shouldChaseAroundBox(nav.brain.my, ball):
            return nav.goLater('chaseAroundBox')
        elif navTrans.shouldAvoidObstacleDuringApproachBall(nav):
            return nav.goLater('avoidObstacle')

    return nav.stay()

def walkSpinToBall(nav):
    ball = nav.brain.ball

    nav.dest = ball
    nav.dest.h = ball.heading

    if navTrans.atDestinationCloserAndFacing(nav.brain.my,
                                             nav.dest,
                                             ball.bearing):
        return nav.goNow('stop')
    elif abs(ball.bearing) > 60:
        return nav.goLater('crossoverTowardsBall')

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
        nav.shouldChaseAroundBox += 1
    else:
        nav.shouldChaseAroundBox = 0

    if nav.shouldChaseAroundBox > constants.STOP_CHASING_AROUND_BOX:
        return nav.goNow('crossoverTowardsBall')

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

MIN_ORBIT_REL_X = 14.
MAX_ORBIT_REL_X = 16.
MAX_DIFF_REL_Y = 2.

def orbitAdjust(nav):
    # needs to adjust far, far less
    # if we need to orbit, switch to orbit state
    ball = nav.brain.ball

    if fabs(ball.relY) < (MAX_DIFF_REL_Y - .5) and \
           ((MIN_ORBIT_REL_X + .5) < ball.relX < (MAX_ORBIT_REL_X - .5)):
        return nav.goNow('orbitBallThruAngle')

    else:
        if fabs(ball.relY) >= (MAX_DIFF_REL_Y - .5):
            sY = MyMath.clip(ball.relY * PFK_Y_GAIN,
                             PFK_MIN_Y_SPEED,
                             PFK_MAX_Y_SPEED)

            sY = max(PFK_MIN_Y_MAGNITUDE,sY) * MyMath.sign(sY)

        else:
            sY = 0.0

        if ((MIN_ORBIT_REL_X +.5) > ball.relX or
            ball.relX >( MAX_ORBIT_REL_X - .5)):
            sX = MyMath.clip((ball.relX - MIN_ORBIT_REL_X )* PFK_X_GAIN,
                             PFK_MIN_X_SPEED,
                             PFK_MAX_X_SPEED)
            sX = max(PFK_MIN_X_MAGNITUDE,sX) * MyMath.sign(sX)

        else:
            sX = 0.0

    helper.setSpeed(nav,sX,sY,0)
    return nav.stay()

def orbitBallThruAngle(nav):
    """
    Circles around a point in front of robot, for a certain angle
    """
    ball = nav.brain.ball
    # ball is too off-center to orbit, center on it
    if nav.firstFrame() and nav.lastDiffState == 'positionForKick':
        nav.orbitFrames = 0
        if fabs(nav.angleToOrbit) < constants.MIN_ORBIT_ANGLE:
            return nav.goNow('positionForKick')

    if fabs(ball.relY) > MAX_DIFF_REL_Y or \
           ball.relX <= MIN_ORBIT_REL_X or \
           ball.relX >= MAX_ORBIT_REL_X:
        print ball.relX
        return nav.goNow('orbitAdjust')

    if nav.firstFrame():
        if nav.angleToOrbit < 0:
            orbitDir = constants.ORBIT_LEFT
        else:
            orbitDir = constants.ORBIT_RIGHT

        if fabs(nav.angleToOrbit) <= constants.ORBIT_SMALL_ANGLE:
            sY = constants.ORBIT_STRAFE_SPEED * constants.ORBIT_SMALL_GAIN
            sT = constants.ORBIT_SPIN_SPEED * constants.ORBIT_SMALL_GAIN

        elif fabs(nav.angleToOrbit) <= constants.ORBIT_LARGE_ANGLE:
            sY = constants.ORBIT_STRAFE_SPEED * \
                constants.ORBIT_MID_GAIN
            sT = constants.ORBIT_SPIN_SPEED * \
                constants.ORBIT_MID_GAIN
        else :
            sY = constants.ORBIT_STRAFE_SPEED * \
                constants.ORBIT_LARGE_GAIN
            sT = constants.ORBIT_SPIN_SPEED * \
                constants.ORBIT_LARGE_GAIN

        walkX = -1.
        walkY = orbitDir*sY
        walkTheta = orbitDir*sT
        helper.setSpeed(nav, walkX, walkY, walkTheta )

    #  (frames/second) / (degrees/second) * degrees + a little b/c we run slow
    framesToOrbit = fabs((constants.FRAME_RATE / nav.walkTheta) *
                         nav.angleToOrbit)
    nav.orbitFrames += 1

    if nav.orbitFrames >= framesToOrbit:
        return nav.goLater('positionForKick')
    return nav.stay()
