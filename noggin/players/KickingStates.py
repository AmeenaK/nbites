"""
Here we house all of the state methods used for kicking the ball
"""

import man.motion.SweetMoves as SweetMoves
import man.motion.HeadMoves as HeadMoves
import man.motion.StiffnessModes as StiffnessModes
import KickingConstants as Constants
import ChaseBallConstants
from .. import NogginConstants
from math import fabs

def decideKick(player):
    """
    Decides which kick to use
    """
    if player.firstFrame():
        player.brain.tracker.switchTo('stopped')
        player.brain.motion.stopHeadMoves()
        player.executeMove(HeadMoves.KICK_SCAN)

        player.kickDecider = KickDecider(player)

    # If scanning, then collect data
    if (player.stateTime < SweetMoves.getMoveTime(HeadMoves.KICK_SCAN)):
        player.kickDecider.collectData(player.brain)
        return player.stay()

    # Done scanning time to act
    player.brain.tracker.trackBall()
    player.kickDecider.calculate()

    if not player.brain.ball.on:
        if player.brain.ball.framesOff < 120: # HACK, should be constant, or better value
            return player.stay()
        else :
            return player.goLater('scanFindBall')

    player.kickDecider.ballForeWhichFoot()

    # Get references to the collected data
    myLeftPostBearing =  player.kickDecider.myLeftPostBearing
    myRightPostBearing = player.kickDecider.myRightPostBearing
    oppLeftPostBearing = player.kickDecider.oppLeftPostBearing
    oppRightPostBearing = player.kickDecider.oppRightPostBearing

    player.printf(player.kickDecider)



    # Things to do if we saw our own goal
    # Saw the opponent goal
    if player.kickDecider.sawOppGoal:
        if oppLeftPostBearing is not None and \
                oppRightPostBearing is not None:
            KICK_STRAIGHT_BEARING_THRESH = 30
            avgOppBearing = (oppLeftPostBearing + oppRightPostBearing)/2
            if fabs(avgOppBearing) < KICK_STRAIGHT_BEARING_THRESH:
                if Constants.DEBUG_KICKS: print ("\t\t Straight 1")
                return player.goLater('kickBallStraight')

            elif avgOppBearing > KICK_STRAIGHT_BEARING_THRESH:
                if Constants.DEBUG_KICKS: print ("\t\t Left 5")
                return player.goLater('kickBallLeft')
            elif avgOppBearing < -KICK_STRAIGHT_BEARING_THRESH:
                if Constants.DEBUG_KICKS: print ("\t\t Right 5")
                return player.goLater('kickBallRight')

        elif oppLeftPostBearing is not None:
            KICK_LEFT_BEARING_THRESH = 60
            KICK_RIGHT_BEARING_THRESH = -10
            if oppLeftPostBearing > KICK_LEFT_BEARING_THRESH:
                if Constants.DEBUG_KICKS: print ("\t\t Left 6.1")
                return player.goLater('kickBallLeft')
            elif oppLeftPostBearing < KICK_RIGHT_BEARING_THRESH:
                if Constants.DEBUG_KICKS: print ("\t\t Left 6.2")
                return player.goLater('kickBallRight')
            else:
                if Constants.DEBUG_KICKS: print ("\t\t Straight 2")
                return player.goLater('kickBallStraight')

        elif oppRightPostBearing is not None:
            KICK_LEFT_BEARING_THRESH = 10
            KICK_RIGHT_BEARING_THRESH = -60
            if oppRightPostBearing > KICK_LEFT_BEARING_THRESH:
                if Constants.DEBUG_KICKS: print ("\t\t Right 6.1")
                return player.goLater('kickBallLeft')
            elif oppRightPostBearing < KICK_RIGHT_BEARING_THRESH:
                if Constants.DEBUG_KICKS: print ("\t\t Right 6.2")
                return player.goLater('kickBallRight')
            else:
                if Constants.DEBUG_KICKS: print ("\t\t Straight 3")
                return player.goLater('kickBallStraight')
        else:
            if Constants.DEBUG_KICKS: print ("\t\t Straight 4")
            return player.goLater('kickBallStraight')

    elif player.kickDecider.sawOwnGoal:
        if Constants.SUPER_SAFE_KICKS:
            return player.goLater('ignoreOwnGoal')

        # We see both posts
        if myLeftPostBearing is not None and myRightPostBearing is not None:
            # Goal in front
            if myRightPostBearing > myLeftPostBearing > 0:
                # kick right
                if Constants.DEBUG_KICKS: print ("\t\tright 1")
                return player.goLater('kickBallRight')
            else:
                # kick left
                if Constants.DEBUG_KICKS: print ("\t\tleft 1")
                return player.goLater('kickBallLeft')

        elif myLeftPostBearing is not None:
            if myLeftPostBearing > 0:
                if Constants.DEBUG_KICKS: print ("\t\tright 2")
                return player.goLater('kickBallRight')
            else:
                if Constants.DEBUG_KICKS: print ("\t\left 2")
                return player.goLater('kickBallLeft')
        elif myRightPostBearing is not None:
            if player.brain.my.h > 0:
                if Constants.DEBUG_KICKS: print ("\t\tright 3")
                return player.goLater('kickBallRight')
            else:
                if Constants.DEBUG_KICKS: print ("\t\tleft 3")
                return player.goLater('kickBallLeft')

            if myRightPostBearing > 0:
                if Constants.DEBUG_KICKS: print ("\t\tleft 4")
                return player.goLater('kickBallLeft')
            else:
                if Constants.DEBUG_KICKS: print ("\t\tright 4")
                return player.goLater('kickBallRight')
        else:
            # don't do anything
            return player.goLater('ignoreOwnGoal')

    else:
        # use localization for kick
        if player.brain.my.h < -Constants.MAX_FORWARD_KICK_ANGLE:
            if Constants.DEBUG_KICKS: print "\t\t Left 7"
            return player.goLater('kickBallLeft')
        elif player.brain.my.h > Constants.MAX_FORWARD_KICK_ANGLE:
            if Constants.DEBUG_KICKS: print "\t\t Right 7"
            return player.goLater('kickBallRight')
        else:
            if Constants.DEBUG_KICKS: print "\t\t Straight 5"
            return player.goLater('kickBallStraight')

def kickBallStraight(player):
    """
    Kick the ball forward.  Currently uses the left foot
    """
    if player.firstFrame():
        player.brain.tracker.trackBall()
        player.printf("We should kick straight!", 'cyan')

        ballForeFoot = player.kickDecider.ballForeFoot
        if ballForeFoot == Constants.LEFT_FOOT or \
                ballForeFoot == Constants.MID_LEFT:
            player.chosenKick = SweetMoves.LEFT_FAR_KICK
            return player.goNow('kickBallExecute')

        elif ballForeFoot == Constants.RIGHT_FOOT or \
                ballForeFoot == Constants.MID_RIGHT:
            player.chosenKick = SweetMoves.RIGHT_FAR_KICK
            return player.goNow('kickBallExecute')

        else :                  # INCORRECT_POS
            return player.goLater('positionForKick')

def kickBallLeft(player):
    """
    Kick the ball to the left, with right foot
    """

    player.chosenKick = SweetMoves.RIGHT_SIDE_KICK
    return player.goNow('sideStepForKick')

def kickBallRight(player):
    """
    Kick the ball to the right, using the left foot
    """
    player.chosenKick = SweetMoves.LEFT_SIDE_KICK
    return player.goNow('sideStepForKick')


def sideStepForKick(player):
    # Which foot is the ball in front of?
    ballForeFoot = player.kickDecider.ballForeFoot

    if ballForeFoot == Constants.MID_RIGHT or \
            ballForeFoot == Constants.MID_LEFT:
        return player.goNow('kickBallExecute')

    # Ball too far outside to kick with
    elif ballForeFoot == Constants.RIGHT_FOOT:
        return player.goNow('stepRightForKick')

    # Ball in front of wrong foot
    elif ballForeFoot == Constants.LEFT_FOOT:
        return player.goNow('stepLeftForKick')

    # Ball must be in wrong place
    else :
        return player.goLater('positionForKick')

def stepRightForKick(player):

    stillStepping = player.setSteps(0,-4,0,5)
    if stillStepping:
        return player.stay()

    return player.goNow('kickBallExecute')

def stepLeftForKick(player):

    stillStepping = player.setSteps(0,4,0,5)
    if stillStepping:
        return player.stay()

    return player.goNow('kickBallExecute')


def kickBallExecute(player):
    if player.firstFrame():
        player.brain.tracker.trackBall()
        player.executeMove(player.chosenKick)

    elif player.stateTime >= SweetMoves.getMoveTime(player.chosenKick):
        return player.goLater('afterKick')

    return player.stay()


def afterKick(player):
    """
    State to follow up after a kick.
    Currently exits after one frame.
    """
    # trick the robot into standing up instead of leaning to the side
    player.walkPose()

    if player.brain.nav.isStopped():
        return player.goLater("chase")
    else:
        return player.stay()

def kickAtPosition(player):
    """
    Method to simply kick while standing in position
    Used for a very simple goalie behavior
    """
    if player.firstFrame():
        player.brain.tracker.trackBall()
        player.executeStiffness(StiffnessModes.LEFT_FAR_KICK_STIFFNESS)
    if player.counter == 2:
        player.executeMove(SweetMoves.LEFT_FAR_KICK)

    if player.stateTime >= SweetMoves.getMoveTime(SweetMoves.LEFT_FAR_KICK):
        player.walkPose()

        if player.brain.nav.isStopped():
            player.brain.CoA.setRobotGait(player.brain.motion)
            player.currentGait = ChaseBallConstants.FAST_GAIT
            return player.goLater('atPosition')

    return player.stay()

class KickDecider:
    """
    Class to hold all the things we need to decide a kick
    """

    def __init__(self,player):
        self.oppGoalLeftPostBearings = []
        self.oppGoalRightPostBearings = []
        self.myGoalLeftPostBearings = []
        self.myGoalRightPostBearings = []

        self.oppLeftPostBearing = None
        self.oppRightPostBearing = None
        self.myLeftPostBearing = None
        self.myRightPostBearing = None

        self.sawOwnGoal = False
        self.sawOppGoal = False

        self.player = player
        self.ballForeFoot = Constants.LEFT_FOOT

    def collectData(self, info):
        """
        Collect info on any observed goals
        """
        if info.myGoalLeftPost.on:
            if info.myGoalLeftPost.certainty == NogginConstants.SURE:
                self.sawOwnGoal = True
                self.myGoalLeftPostBearings.append(info.myGoalLeftPost.bearing)

        if info.myGoalRightPost.on:
            if info.myGoalRightPost.certainty == NogginConstants.SURE:
                self.sawOwnGoal = True
                self.myGoalRightPostBearings.append(info.myGoalRightPost.bearing)

        if info.oppGoalLeftPost.on:
            if info.oppGoalLeftPost.certainty == NogginConstants.SURE:
                self.sawOppGoal = True
                self.oppGoalLeftPostBearings.append(info.oppGoalLeftPost.bearing)

        if info.oppGoalRightPost.on:
            if info.oppGoalRightPost.certainty == NogginConstants.SURE:
                self.sawOppGoal = True
                self.oppGoalRightPostBearings.append(info.oppGoalRightPost.bearing)

    def calculate(self):
        """
        Get usable data from the collected data
        """
        if len(self.myGoalLeftPostBearings) > 0:
            self.myLeftPostBearing = (sum(self.myGoalLeftPostBearings) /
                                      len(self.myGoalLeftPostBearings))
        if len(self.myGoalRightPostBearings) > 0:
            self.myRightPostBearing = (sum(self.myGoalRightPostBearings) /
                                       len(self.myGoalRightPostBearings))
        if len(self.oppGoalLeftPostBearings) > 0:
            self.oppLeftPostBearing = (sum(self.oppGoalLeftPostBearings) /
                                       len(self.oppGoalLeftPostBearings))
        if len(self.oppGoalRightPostBearings) > 0:
            self.oppRightPostBearing = (sum(self.oppGoalRightPostBearings) /
                                        len(self.oppGoalRightPostBearings))

    def ballForeWhichFoot(self):
        ball = self.player.brain.ball

        if Constants.LEFT_FOOT_L_Y > ball.relY >= Constants.LEFT_FOOT_R_Y:
            self.ballForeFoot = Constants.LEFT_FOOT

        elif Constants.LEFT_FOOT_R_Y > ball.relY >= 0:
            self.ballForeFoot = Constants.MID_LEFT

        elif 0 > ball.relY > Constants.RIGHT_FOOT_L_Y:
            self.ballForeFoot = Constants.MID_RIGHT

        elif Constants.RIGHT_FOOT_L_Y > ball.relY > Constants.RIGHT_FOOT_R_Y:
            self.ballForeFoot = Constants.RIGHT_FOOT

        else:
            self.ballForeFoot = Constants.INCORRECT_POS

    def __str__(self):
        s = ""
        if self.myLeftPostBearing is not None:
            s += ("My left post bearing is: " + str(self.myLeftPostBearing) + "\n")
        if self.myRightPostBearing is not None:
            s += ("My right post bearing is: " + str(self.myRightPostBearing) + "\n")
        if self.oppLeftPostBearing is not None:
            s += ("Opp left post bearing is: " + str(self.oppLeftPostBearing) + "\n")
        if self.oppRightPostBearing is not None:
            s += ("Opp right post bearing is: " + str(self.oppRightPostBearing) + "\n")
        if s == "":
            s = "No goal posts observed"
        return s
