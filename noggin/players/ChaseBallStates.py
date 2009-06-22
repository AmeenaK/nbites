"""
Here we house all of the state methods used for chasing the ball
"""

from man.noggin.util import MyMath
from man.motion import SweetMoves
import ChaseBallConstants as constants
import ChaseBallTransitions as transitions
import PositionConstants
from .. import NogginConstants
from math import fabs

def chase(player):
    """
    Method to determine which chase state should be used.
    We dump the robot into this state when we our switching from something else.
    """
    if transitions.shouldScanFindBall(player):
        return player.goNow('scanFindBall')
    elif transitions.shouldApproachBallWithLoc(player):
        return player.goNow('approachBallWithLoc')
    elif transitions.shouldApproachBall(player):
        return player.goNow('approachBall')
    elif transitions.shouldKick(player):
        return player.goNow('waitBeforeKick')
    elif transitions.shouldTurnToBall_ApproachBall(player):
        return player.goNow('turnToBall')
    elif transitions.shouldSpinFindBall(player):
        return player.goNow('spinFindBall')
    else:
        return player.goNow('scanFindBall')

def chaseAfterKick(player):
    if player.firstFrame():
        player.brain.tracker.trackBall()

        if player.chosenKick == SweetMoves.LEFT_FAR_KICK or \
                player.chosenKick == SweetMoves.RIGHT_FAR_KICK:
            return player.goLater('chase')

        if player.chosenKick == SweetMoves.LEFT_SIDE_KICK:
            turnDir = constants.TURN_RIGHT

        elif player.chosenKick == SweetMoves.RIGHT_SIDE_KICK:
            turnDir = constants.TURN_LEFT

        player.setSpeed(0, 0, turnDir * constants.BALL_SPIN_SPEED)
        return player.stay()

    if player.brain.ball.framesOn > constants.BALL_ON_THRESH:
        return player.goLater('chase')
    elif player.counter > constants.CHASE_AFTER_KICK_FRAMES:
        return player.goLater('spinFindBall')
    return player.stay()

def turnToBall(player):
    """
    Rotate to align with the ball. When we get close, we will approach it
    """
    ball = player.brain.ball

    if player.firstFrame():
        player.brain.tracker.trackBall()

    # Determine the speed to turn to the ball
    turnRate = MyMath.clip(ball.bearing*constants.BALL_SPIN_GAIN,
                           -constants.BALL_SPIN_SPEED,
                           constants.BALL_SPIN_SPEED)

    # Avoid spinning so slowly that we step in place
    if fabs(turnRate) < constants.MIN_BALL_SPIN_SPEED:
        turnRate = MyMath.sign(turnRate)*constants.MIN_BALL_SPIN_SPEED

    if transitions.shouldKick(player):
        return player.goNow('waitBeforeKick')
    elif transitions.shouldPositionForKick(player):
        return player.goNow('positionForKick')
    elif transitions.shouldApproachBall(player):
        return player.goLater('approachBall')
    elif transitions.shouldScanFindBall(player):
        return player.goLater('scanFindBall')

    elif ball.on:
        player.setSpeed(x=0,y=0,theta=turnRate)

    return player.stay()

def approachBallWithLoc(player):
    nav = player.brain.nav
    my = player.brain.my

    if transitions.shouldKick(player):
        return player.goNow('waitBeforeKick')
    elif transitions.shouldPositionForKickFromApproachLoc(player):
        return player.goLater('positionForKick')
    elif transitions.shouldAvoidObstacle(player):
        return player.goLater('avoidObstacle')
    #elif my.locScoreFramesBad > constants.APPROACH_NO_LOC_THRESH:
        #return player.goLater('approachBall')
    elif not player.brain.tracker.activeLocOn and \
            transitions.shouldScanFindBall(player):
        return player.goLater('scanFindBall')
    elif player.brain.tracker.activeLocOn and \
            transitions.shouldScanFindBallActiveLoc(player):
        return player.goLater('scanFindBall')

    nav = player.brain.nav
    my = player.brain.my

    if player.brain.ball.locDist > constants.APPROACH_ACTIVE_LOC_DIST:
        player.brain.tracker.activeLoc()
    else :
        player.brain.tracker.trackBall()

    dest = player.getApproachPosition()
    useOmni = MyMath.dist(my.x, my.y, dest[0], dest[1]) <= \
        constants.APPROACH_OMNI_DIST
    changedOmni = False

    if useOmni != nav.movingOmni:
        player.changeOmniGoToCounter += 1
    else :
        player.changeOmniGoToCounter = 0
    if player.changeOmniGoToCounter > PositionConstants.CHANGE_OMNI_THRESH:
        changedOmni = True

    if player.firstFrame() or \
            nav.destX != dest[0] or \
            nav.destY != dest[1] or \
            nav.destH != dest[2] or \
            changedOmni:
        if not useOmni:
            nav.goTo(dest)
        else:
            nav.omniGoTo(dest)

    return player.stay()


def approachBall(player):
    """
    Once we are alligned with the ball, approach it
    """
    if player.firstFrame():
        player.brain.tracker.trackBall()

    if player.penaltyKicking and \
            player.ballInOppGoalBox():
        player.stopWalking()
        return player.stay()

    # Switch to other states if we should
    if transitions.shouldKick(player):
        return player.goNow('waitBeforeKick')
    elif transitions.shouldPositionForKick(player):
        return player.goNow('positionForKick')
    elif transitions.shouldTurnToBall_ApproachBall(player):
        return player.goLater('turnToBall')
    elif transitions.shouldScanFindBall(player):
        return player.goLater('scanFindBall')
    elif transitions.shouldAvoidObstacle(player):
        return player.goLater('avoidObstacle')

    # Determine our speed for approaching the ball
    ball = player.brain.ball
    sX = MyMath.clip(ball.dist*constants.APPROACH_X_GAIN,
                     constants.MIN_APPROACH_X_SPEED,
                     constants.MAX_APPROACH_X_SPEED)

    # Determine the speed to turn to the ball
    sTheta = MyMath.clip(ball.bearing*constants.APPROACH_SPIN_GAIN,
                         -constants.APPROACH_SPIN_SPEED,
                         constants.APPROACH_SPIN_SPEED)
    # Avoid spinning so slowly that we step in place
    if fabs(sTheta) < constants.MIN_APPROACH_SPIN_SPEED:
        sTheta = 0.0

    # Set our walk towards the ball
    if ball.on:
        player.setSpeed(sX,0,sTheta)

    return player.stay()

def positionForKick(player):
    """
    State to align on the ball once we are near it
    """
    ball = player.brain.ball

    # Leave this state if necessary
    if transitions.shouldKick(player):
        return player.goNow('waitBeforeKick')
    elif transitions.shouldScanFindBall(player):
        return player.goLater('scanFindBall')
    elif transitions.shouldTurnToBallFromPositionForKick(player):
        return player.goLater('turnToBall')
    elif transitions.shouldApproachFromPositionForKick(player):
        return player.goLater('approachBall')

    # Determine approach speed
    targetY = ball.relY

    sY = MyMath.clip(targetY * constants.PFK_Y_GAIN,
                     constants.PFK_MIN_Y_SPEED,
                     constants.PFK_MAX_Y_SPEED)
    if fabs(sY) < constants.PFK_MIN_Y_MAGNITUDE:
        sY = 0.0

    if transitions.shouldApproachForKick(player):
        targetX = (ball.relX -
                   (constants.BALL_KICK_LEFT_X_CLOSE +
                    constants.BALL_KICK_LEFT_X_FAR) / 2.0)
        sX = MyMath.clip(ball.relX * constants.PFK_X_GAIN,
                         constants.PFK_MIN_X_SPEED,
                         constants.PFK_MAX_X_SPEED)
    else:
        sX = 0.0

#     player.printf("\tPosition for kick target_y is " +
#                   str(targetY), "cyan")
#     player.printf("\tPosition for kick sY is " + str(sY), "cyan")
#     player.printf("\tPosition for kick sX is " + str(sX), "cyan")
#     player.printf("\tBall dist is: " + str(ball.dist), "cyan")
#     player.printf("\tBall current y is: " +
#                   str(ball.locRelY) + " want between " +
#                   str(constants.BALL_KICK_LEFT_Y_L) +
#                   " and " +
#                   str(constants.BALL_KICK_LEFT_Y_R), "cyan")
#     player.printf("\tBall current x is: " +
#                   str(ball.locRelX) + " want between " +
#                   str(constants.BALL_KICK_LEFT_X_CLOSE) +
#                   " and " + str(constants.BALL_KICK_LEFT_X_FAR), "cyan")

    if ball.on:
        player.setSpeed(sX,sY,0)

    return player.stay()

def waitBeforeKick(player):
    """
    Stop before we kick to make sure we want to kick
    """
    if player.firstFrame():
        player.stopWalking()

    if not player.brain.nav.isStopped():
        return player.stay()
    elif transitions.shouldApproachForKick(player):
        player.brain.tracker.trackBall()
        return player.goLater('approachBall')
    elif transitions.shouldScanFindBall(player):
        player.brain.tracker.trackBall()
        return player.goLater('scanFindBall')
    elif transitions.shouldRepositionForKick(player):
        player.brain.tracker.trackBall()
        return player.goLater('positionForKick')
    else:
        return player.goLater('getKickInfo')

def ignoreOwnGoal(player):
    """
    Method to intelligently ignore kicking into our own goal
    """
    if transitions.shouldSpinFindBall(player):
        return player.goNow('spinFindBall')

    return player.stay()

def avoidObstacle(player):
    """
    If we detect something in front of us, dodge it
    """
    if player.firstFrame():
        player.doneAvoidingCounter = 0
        player.printf(player.brain.sonar)


        if (transitions.shouldAvoidObstacleLeft(player) and
            transitions.shouldAvoidObstacleRight(player)):
            # Backup
            player.setSpeed(constants.DODGE_LEFT_SPEED, 0, 0)

        elif transitions.shouldAvoidObstacleLeft(player):
            # Dodge right
            player.setSpeed(0, constants.DODGE_RIGHT_SPEED, 0)

        elif transitions.shouldAvoidObstacleRight(player):
            # Dodge left
            player.setSpeed(0, constants.DODGE_LEFT_SPEED, 0)

    if not transitions.shouldAvoidObstacle(player):
        player.doneAvoidingCounter += 1
    else :
        player.doneAvoidingCounter -= 1
        player.doneAvoidingCounter = max(0, player.doneAvoidingCounter)

    if player.doneAvoidingCounter > constants.DONE_AVOIDING_FRAMES_THRESH:
        player.shouldAvoidObstacleRight = 0
        player.shouldAvoidObstacleLeft = 0
        player.stopWalking()
        return player.goLater(player.lastDiffState)

    return player.stay()

def orbitBall(player):
    """
    Method to spin around the ball before kicking
    """
    if player.firstFrame():
        player.brain.tracker.trackBall()
        player.brain.nav.orbitAngle(player.orbitAngle)

    if player.brain.nav.isStopped():
        return player.goLater('getKickInfo')

    return player.stay()

def steps(player):
    if player.brain.nav.isStopped():
        player.setSteps(3,3,0,5)
    elif player.brain.nav.currentState != "stepping":
        player.stopWalking()
    return player.stay()
