from .. import NogginConstants
from . import ChaseBallConstants as ChaseConstants
import man.noggin.util.MyMath as MyMath
import PositionTransitions as transitions
import PositionConstants as constants

def positionLocalize(player):
    """
    Localize better in order to position
    """
    return player.stay()

def playbookPosition(player):
    """
    Have the robot navigate to the position reported to it from playbook
    """
    brain = player.brain
    nav = brain.nav
    my = brain.my
    ball = brain.ball
    gcState = brain.gameController.currentState

    if player.firstFrame():
        nav.positionPlaybook()

        if gcState == 'gameReady':
            brain.tracker.locPans()
        else:
            brain.tracker.activeLoc()

    return player.stay()

def relocalize(player):
    if player.firstFrame():
        player.setWalk(10 , 0, 0)

    if player.brain.my.locScore == NogginConstants.GOOD_LOC or \
            player.brain.my.locScore == NogginConstants.OK_LOC:
        player.shouldRelocalizeCounter += 1

        if player.shouldRelocalizeCounter > 30:
            player.shouldRelocalizeCounter = 0
            return player.goLater(player.lastDiffState)

    else:
        player.shouldRelocalizeCounter = 0

    if not player.brain.motion.isHeadActive():
        player.brain.tracker.locPans()

    if player.counter > constants.RELOC_SPIN_FRAME_THRESH:
        direction = MyMath.sign(player.getWalk()[2])
        if direction == 0:
            direction = 1

        player.setWalk(0 , 0, constants.RELOC_SPIN_SPEED * direction)

    return player.stay()

def afterPenalty(player):

    if player.lastDiffState == 'relocalize':
        return player.goNow('gamePlaying')

    if player.firstFrame():
        lookCount = 0 #0 for none, 1 for left, 2 for right
        frameCount = 0
        player.brain.tracker.performHeadMove(LOOK_UP_LEFT)
        lookCount = 1

    if not player.brain.motion.isHeadActive() and lookCount == 1:
        ##looking left

        if player.brain.yglp.on or player.brain.ygrp.on:
            #set loc info
            player.brain.loc.resetLocTo(player.brain.Constants.CENTER_FIELD_X, \
                            player.brain.Constants.FIELD_WHITE_TOP_SIDELINE_Y, \
                            -90.0)
            return player.goNow(player.lastDiffState)

        if player.brain.bglp.on or player.brain.bgrp.on:
            player.brain.loc.resetLocTo(player.brain.Constants.Center_FIELD_X, \
                            player.brain.Constants.FIELD_WHITE_BOTTOM_SIDELINE_Y \
                            90.0)
            return player.goNow(player.lastDiffState)

        if player.brain.ball.on:
            #go to it and don't worry about loc
            return player.goNow('chase')

        frameCount += 1

    if frameCount > 10:
        frameCount = 0
        lookCount = 2
        player.brain.tracker.performHeadMove(LOOK_UP_RIGHT)

    if not player.brain.motion.isHeadActive() and lookCount == 2:
        ##looking right

        if player.brain.yglp.on or player.brain.ygrp.on:
            player.brain.loc.resetLocTo(player.brain.Constants.Center_FIELD_X, \
                            player.brain.Constants.FIELD_WHITE_BOTTOM_SIDELINE_Y \
                            90.0)
            return player.goNow(player.lastDiffState)

        if player.brain.bglp.on or player.brain.bgrp.on:
            player.brain.loc.resetLocTo(player.brain.Constants.CENTER_FIELD_X, \
                            player.brain.Constants.FIELD_WHITE_TOP_SIDELINE_Y, \
                            -90.0)
            return player.goNow(player.lastDiffState)

        if player.brain.ball.on:
            #go to it and don't worry about loc
            return player.goNow('chase')

        frameCount += 1

    if frameCount > 10:
        return player.goNow('relocalize')

    return player.stay()
