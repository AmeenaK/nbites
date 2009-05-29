from .. import NogginConstants as Constants
from . import ChaseBallConstants as ChaseConstants
import ChaseBallTransitions as chaseTransitions
import man.noggin.util.MyMath as MyMath
import man.motion.SweetMoves as SweetMoves
import man.motion.StiffnessModes as StiffnessModes

def positionLocalize(player):

    return player.stay()

def playbookPosition(player):
    position = player.brain.playbook.position
    if player.firstFrame():
        player.stopWalking()
        player.brain.tracker.activeLoc()
        player.printf("I am going to " + str(player.brain.playbook.position))
        player.brain.nav.goTo(position[0], position[1], Constants.OPP_GOAL_HEADING)

    if player.brain.nav.destX != position[0] or \
            player.brain.nav.destY != position[1]:
        player.brain.nav.goTo(position[0], position[1], Constants.OPP_GOAL_HEADING)
        #player.printf("position = "+str(position[0])+" , "+str(position[1]) )

    # we're at the point, let's switch to another state
    if player.brain.nav.isStopped() and player.counter > 0:
        return player.goLater('atPosition')

    return player.stay()

def positionOnBall(player):
    # for now we want a way to not rely on localization for chasing
    if not ChaseConstants.USE_LOC_CHASE:
        if chaseTransitions.shouldApproachBall(player):
            return player.goLater('approachBall')
        elif chaseTransitions.shouldTurnToBall_ApproachBall(player):
            return player.goLater('turnToBallFar')
        elif chaseTransitions.shouldSpinFindBall(player):
            return player.goLater('spinFindBall')
        elif chaseTransitions.shouldTurnToBallClose(player):
            return player.goLater('turnToBallClose')
        else :
            return player.goLater('scanFindBall')

    nextX, nextY,nextH = player.getBehindBallPosition()

    if player.firstFrame():
        player.brain.nav.goTo(nextX,nextY,nextH)

    #player.printf("position = "+str(nextX)+" , "+str(nextY) )
    if player.brain.nav.destX != nextX or \
            player.brain.nav.destY != nextY:
        player.brain.nav.goTo(nextX,nextY,nextH)

    if player.brain.nav.isStopped():
        return player.goLater('chase')

    return player.stay()

def atPosition(player):
    """
    State for when we're at the position
    """
    if player.firstFrame():
        player.stopWalking()
        player.brain.tracker.trackBall()
    if 0 < player.brain.ball.dist < 25:
        return player.goLater('kickAtPosition')
    return player.stay()

def spinToBall(player):
    """
    State to spin to turn to the ball
    """
    if player.firstFrame():
        player.stopWalking()
        player.brain.tracker.trackBall()
    ball = player.brain.ball

    turnRate = MyMath.clip(ball.locBearing*ChaseConstants.BALL_SPIN_GAIN,
                           -ChaseConstants.BALL_SPIN_SPEED,
                           ChaseConstants.BALL_SPIN_SPEED)

    if chaseTransitions.shouldBeAtSpinDir(player):
        return player.goLater('atSpinBallPosition')

    elif chaseTransitions.shouldSpinFindBallPosition(player):
        return player.goLater('spinFindBallPosition')

    elif player.currentSpinDir != MyMath.sign(turnRate):
        player.stopWalking()
        player.currentSpinDir = MyMath.sign(turnRate)
    elif player.stoppedWalk and ball.on and player.brain.nav.isStopped():
        player.brain.CoA.setRobotTurnGait(player.brain.motion)
        player.setSpeed(x=0,y=0,theta=turnRate)

    return player.stay()

def atSpinBallPosition(player):
    """
    Spun to the ball heading, spin again
    """
    if player.firstFrame():
        player.stopWalking()
        player.brain.tracker.activeLoc()

    if chaseTransitions.shouldTurnToBall_fromAtBallPosition(player):
        return player.goLater('spinToBall')
    elif chaseTransitions.shouldSpinFindBallPosition(player):
        return player.goLater('spinFindBallPosition')

    return player.stay()

def spinFindBallPosition(player):
    """
    Spin to find the ball if it is not being seen.
    """
    if player.brain.nav.isStopped():
        player.stoppedWalk = True

    if player.firstFrame() and player.stoppedWalk:
        player.setSpeed(0,
                        0,
                        ChaseConstants.FIND_BALL_SPIN_SPEED)
        player.brain.tracker.trackBall()


    if chaseTransitions.shouldTurnToBall_fromAtBallPosition(player):
        return player.goLater('spinToBall')
    if chaseTransitions.shouldTurnToBall_ApproachBall(player):
        return player.goLater('atSpinBallPosition')

    return player.stay()


def kickAtPosition(player):
    if player.firstFrame():
        player.brain.tracker.trackBall()
        player.executeStiffness(StiffnessModes.LEFT_FAR_KICK_STIFFNESS)
    if player.counter == 2:
        player.executeMove(SweetMoves.LEFT_FAR_KICK)

    if player.stateTime >= SweetMoves.getMoveTime(SweetMoves.LEFT_FAR_KICK):
        # trick the robot into standing up instead of leaning to the side
        player.executeStiffness(StiffnessModes.LOOSE_ARMS_STIFFNESSES)
        player.setSpeed(0,0,0)
        return player.goLater('atPosition')

    return player.stay()
