from .. import NogginConstants
from . import kicks
from . import KickingConstants as constants
from math import fabs

class KickDecider(object):
    """
    uses current information when called to determine what the best possible
    kick is and where we need to be to execute it
    """

    def __init__(self, brain):
        self.brain = brain
        self.hasKickedOff = True
        self.objDict = { constants.OBJECTIVE_CLEAR:self.kickoff,
                         constants.OBJECTIVE_CENTER:self.center,
                         constants.OBJECTIVE_SHOOT:self.shoot,
                         constants.OBJECTIVE_KICKOFF:self.kickoff }
        self.kickDest = None
        self.destDist = 0.
        self.currentKick = None

    def getSweetMove(self):
        """
        returns the proper sweet move to execute to kick
        """
        ball = self.brain.ball
        return self.currentKick.sweetMove(ball.relX, ball.relY,
                                          self.destDist)

    def decideKick(self):
        """
        using objective and localization determines best kick to make
        """

        objective = self.getObjective()

        # uses dictionary to retrieve and call proper method
        self.kickDest = self.objDict[objective]()
        # take my position and destination of kick to decide which kick
        # need to consider: distance to kick, time needed to align for kick
        # prioritize time to align
        # calculate bearing to dest
        bearing = self.brain.my.getRelativeBearing(self.kickDest)
        if fabs(bearing) >= 180.:
            print "kick sideways"

            # kick sideways
            # left or right?
            # positive bearing means dest is to my left, so kick right
            if bearing > 0:
                kick = kicks.RIGHT_SIDE_KICK
                kick.heading = self.brain.ball.headingTo(self.kickDest) - 90.
            else:
                kick = kicks.LEFT_SIDE_KICK
                kick.heading = self.brain.ball.headingTo(self.kickDest) + 90.

        else:
            print "kick straight"
            #kick straight
            kick = kicks.DYNAMIC_STRAIGHT_KICK
            kick.heading = self.brain.ball.headingTo(self.kickDest)

        self.destDist = self.brain.ball.distTo(self.kickDest)
        self.currentKick = kick
        self.brain.out.printf(self.currentKick)

    def getObjective(self):
        """
        determines what we want to do with the ball
        """
        ball = self.brain.ball

        if not self.hasKickedOff:
            return constants.OBJECTIVE_KICKOFF

        # if ball on our side, get it out!
        if ball.x < NogginConstants.CENTER_FIELD_X:
            # clear (or maybe pass?)
            return constants.OBJECTIVE_CLEAR

        # if deep in opponent corner, center the ball
        elif (ball.x > NogginConstants.OPP_GOALBOX_LEFT_X and
              (ball.y > NogginConstants.OPP_GOALBOX_TOP_Y or
               ball.y < NogginConstants.OPP_GOALBOX_BOTTOM_Y)):
            return constants.OBJECTIVE_CENTER

        return constants.OBJECTIVE_SHOOT

    def kickoff(self):
        """returns a destination for kickoff kick """
        print "kickoff"
        self.hasKickedOff = True
        return constants.LEFT_KICKOFF_POINT

    def clear(self):
        """chooses whether to use left or right clear destination """
        ball = self.brain.ball
        if ball.y < NogginConstants.CENTER_FIELD_Y:
            print "clear left"
            return constants.LEFT_CLEAR_POINT
        else:
            print "clear right"
            return constants.RIGHT_CLEAR_POINT

    def center(self):
        """ returns point to center ball"""
        print "center the ball"
        return constants.CENTER_BALL_POINT

    def shoot(self):
        """ returns best location to shoot at"""
        # TODO: use vision info to find and shoot at gaps

        ball = self.brain.ball
        if ball.y < NogginConstants.CENTER_FIELD_Y:
            print "shoot left"
            return constants.SHOOT_AT_LEFT_AIM_POINT
        else:
            print "shoot right"
            return constants.SHOOT_AT_RIGHT_AIM_POINT
