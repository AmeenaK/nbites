from ..playbook import PBConstants
from .. import NogginConstants
import man.motion.SweetMoves as SweetMoves
import man.noggin.util.MyMath as MyMath
import GoalieTransitions as helper

CENTER_SAVE_THRESH = 15.
ORTHO_GOTO_THRESH = NogginConstants.CENTER_FIELD_X/2
STRAFE_ONLY = False
STRAFE_SPEED = 6
STRAFE_STEPS = 5

def goaliePosition(player):
    if helper.shouldSave(player):
        return player.goNow('goalieSave')
    if helper.shouldChase(player):
        return player.goLater('goalieChase')
    brain = player.brain
    ball = brain.ball
    nav = brain.nav

    if 0 < ball.locDist <= PBConstants.BALL_LOC_LIMIT*(3./4.):
        brain.tracker.trackBall()
    else:
        brain.tracker.activeLoc()

    position = player.brain.playbook.position
    useOrtho = True #(MyMath.dist(brain.my.x, brain.my.y, position[0],
                                   #position[1]) <= ORTHO_GOTO_THRESH)
    if ball.on:
        relY = ball.relY
    else:
        relY = ball.locRelY

    if player.firstFrame():
        if STRAFE_ONLY:
            if relY > CENTER_SAVE_THRESH and nav.isStopped():
                nav.setSteps(0, STRAFE_SPEED, 0, STRAFE_STEPS)
                nav.switchTo('stepping')
            elif relY < -CENTER_SAVE_THRESH and nav.isStopped():
                nav.setSteps(0, -STRAFE_SPEED, 0, STRAFE_STEPS)
                nav.switchTo('stepping')
        else:
            if useOrtho:
                nav.orthoGoTo(position[0], position[1],
                              NogginConstants.OPP_GOAL_HEADING)
            else:
                nav.goTo(position[0], position[1],
                         NogginConstants.OPP_GOAL_HEADING)
    elif nav.destX != position[0] or nav.destY != position[1] or\
        useOrtho!=nav.movingOrtho:
        if STRAFE_ONLY:
            if relY > CENTER_SAVE_THRESH and nav.isStopped():
                nav.setSteps(0, STRAFE_SPEED, 0, STRAFE_STEPS)
                nav.switchTo('stepping')
            elif relY < -CENTER_SAVE_THRESH and nav.isStopped():
                nav.setSteps(0, -STRAFE_SPEED, 0, STRAFE_STEPS)
                nav.switchTo('stepping')
        else:
           if useOrtho:
               nav.orthoGoTo(position[0], position[1],
                             NogginConstants.OPP_GOAL_HEADING)
           else:
               nav.goTo(position[0], position[1],
                        NogginConstants.OPP_GOAL_HEADING)
        # we're at the point, let's switch to another state
    if nav.isStopped() and player.counter > 0:
        return player.goLater('goalieAtPosition')

    return player.stay()

def goalieAtPosition(player):
    """
    State for when we're at the position
    """
    brain = player.brain
    if helper.shouldSave(player):
        return player.goNow('goalieSave')
    if helper.shouldChase(player):
        return player.goLater('goalieChase')

    if 0 < brain.ball.locDist <= PBConstants.BALL_LOC_LIMIT:
        brain.tracker.trackBall()
    else:
        brain.tracker.activeLoc()

    if brain.ball.on:
        relY = brain.ball.relY
    else:
        relY = brain.ball.locRelY

    if STRAFE_ONLY:
        if player.brain.nav.isStopped() and\
                abs(brain.ball.locRelY) > CENTER_SAVE_THRESH:
            return player.goLater('goaliePosition')
    elif brain.nav.notAtHeading(brain.nav.destH) or\
            not brain.nav.atDestinationCloser():
        return player.goLater('goaliePosition')

    return player.stay()

def goalieSave(player):
    player.brain.tracker.trackBall()
    ball = player.brain.ball
    # Figure out where the ball is going and when it will be there
    if ball.on:
        relX = ball.relX
        relY = ball.relY
    else:
        relX = ball.locRelX
        relY = ball.locRelY
    # Decide the type of save
    if relY > CENTER_SAVE_THRESH:
        print "Should be saving left"
        return player.goNow('saveLeft')
    elif relY < -CENTER_SAVE_THRESH:
        print "Should be saving right"
        return player.goNow('saveRight')
    else:
        print "Should be saving center"
        return player.goNow('saveCenter')

def saveRight(player):
    if player.firstFrame():
        player.executeMove(SweetMoves.SAVE_RIGHT_DEBUG)
    if player.stateTime >= SweetMoves.getMoveTime(SweetMoves.SAVE_RIGHT_DEBUG):
        return player.goLater('holdRightSave')
    return player.stay()

def saveLeft(player):
    if player.firstFrame():
        player.executeMove(SweetMoves.SAVE_LEFT_DEBUG)
    if player.stateTime >= SweetMoves.getMoveTime(SweetMoves.SAVE_LEFT_DEBUG):
        return player.goLater('holdLeftSave')
    return player.stay()

def saveCenter(player):
    if player.firstFrame():
        player.executeMove(SweetMoves.SAVE_CENTER_DEBUG)
    if player.stateTime >= SweetMoves.getMoveTime(SweetMoves.SAVE_CENTER_DEBUG):
        return player.goLater('holdCenterSave')
    return player.stay()

def holdRightSave(player):
    if helper.shouldHoldSave(player):
        player.executeMove(SweetMoves.SAVE_RIGHT_HOLD_DEBUG)
    else:
        return player.goLater('postSave')
    return player.stay()

def holdLeftSave(player):
    if helper.shouldHoldSave(player):
        player.executeMove(SweetMoves.SAVE_LEFT_HOLD_DEBUG)
    else:
        return player.goLater('postSave')
    return player.stay()

def holdCenterSave(player):
    if helper.shouldHoldSave(player):
        player.executeMove(SweetMoves.SAVE_CENTER_HOLD_DEBUG)
    else:
        return player.goLater('postSave')
    return player.stay()

def postSave(player):
    if player.firstFrame():
        player.executeMove(SweetMoves.INITIAL_POS)
        player.brain.tracker.trackBall()
    if player.stateTime >= SweetMoves.getMoveTime(SweetMoves.INITIAL_POS):
        roleState = player.getRoleState(player.currentRole)
        return player.goLater(roleState)

    return player.stay()
