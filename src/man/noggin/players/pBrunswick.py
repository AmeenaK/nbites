from . import SoccerFSA
from . import ChaseBallStates
from . import PositionStates
from . import FindBallStates
from . import KickingStates
from . import PenaltyKickStates
from . import GoaliePositionStates
from . import GoalieSaveStates
from . import BrunswickStates

from . import GoalieTransitions
from . import ChaseBallTransitions
from . import KickingHelpers

from .. import NogginConstants
from ..playbook import PBConstants
from . import ChaseBallConstants as ChaseConstants

from man.motion import SweetMoves
from man.noggin.typeDefs.Location import Location, RobotLocation

from math import sin, cos, radians

class SoccerPlayer(SoccerFSA.SoccerFSA):
    def __init__(self, brain):
        SoccerFSA.SoccerFSA.__init__(self,brain)
        self.addStates(PenaltyKickStates)
        self.addStates(GoaliePositionStates)
        self.addStates(GoalieSaveStates)
        self.addStates(PositionStates)
        self.addStates(FindBallStates)
        self.addStates(KickingStates)
        self.addStates(ChaseBallStates)
        self.addStates(BrunswickStates)

        self.setName('pBrunswick')

        self.chosenKick = None
        self.kickDecider = None
        self.justKicked = False
        self.inKickingState = False

        self.shouldSaveCounter = 0
        self.shouldChaseCounter = 0
        self.shouldStopCaseCounter = 0
        self.posForSaveCounter = 0
        self.framesFromCenter = 0
        self.stepsOffCenter = 0
        self.isChasing = False
        self.saving = False

        self.shouldRelocalizeCounter = 0

        self.angleToAlign = 0.0
        self.orbitAngle = 0.0
        self.hasAlignedOnce = False
        self.bigKick = False

        self.kickObjective = None

        # Penalty kick player variables
        self.penaltyKicking = False
        self.penaltyMadeFirstKick = True
        self.penaltyMadeSecondKick = False

        # Kickoff kick
        self.hasKickedOffKick = True

    def run(self):
        self.play = self.brain.play
        if self.currentState == 'afterKick' or \
                self.lastDiffState == 'afterKick':
            self.justKicked = True
        else:
            self.justKicked = False

        gcState = self.brain.gameController.currentState
        if gcState == 'gamePlaying' or\
                (gcState == 'penaltyShotsGamePlaying'
                 and self.play.isRole(PBConstants.GOALIE)):
            roleState = self.getNextState()

            if roleState != self.currentState:
                self.brain.CoA.setRobotGait(self.brain.motion)
                self.switchTo(roleState)

        SoccerFSA.SoccerFSA.run(self)

    def getNextState(self):
        if self.play.isRole(PBConstants.GOALIE):
            state = GoalieTransitions.goalieRunChecks(self)
            return state

        elif self.brain.playbook.subRoleUnchanged():
            return self.currentState

        elif self.inKickingState:
            return self.currentState

        else:
            return self.getRoleState()

    def getRoleState(self):
        if self.play.isRole(PBConstants.CHASER):
            return 'chase'
        elif self.play.isRole(PBConstants.PENALTY_ROLE):
            return 'gamePenalized'
        else:
            return 'playbookPosition'


    ###### HELPER METHODS ######
    def getSpinDirAfterKick(self):
        if self.chosenKick == SweetMoves.LEFT_SIDE_KICK:
            return ChaseConstants.TURN_RIGHT
        elif self.chosenKick == SweetMoves.RIGHT_FAR_KICK:
            return ChaseConstants.TURN_LEFT
        else :
            return ChaseConstants.TURN_LEFT

    def inFrontOfBall(self):
        ball = self.brain.ball
        my = self.brain.my
        return my.x > ball.x and \
            my.y < ChaseConstants.IN_FRONT_SLOPE*(my.x - ball.x) + ball.y and \
            my.y > -ChaseConstants.IN_FRONT_SLOPE*(my.x-ball.x) + ball.y

    def getApproachPosition(self):
        ball = self.brain.ball
        my = self.brain.my

        if self.penaltyKicking:
            destKickLoc = self.getPenaltyKickingBallDest()
            ballLoc = RobotLocation(ball.x, ball.y, NogginConstants.OPP_GOAL_HEADING)
            destH = my.getRelativeBearing(destKickLoc)

        elif self.inFrontOfBall():
            destH = self.getApproachHeadingFromFront()
        elif self.shouldMoveAroundBall():
            return self.getPointToMoveAroundBall()
        else :
            destH = self.getApproachHeadingFromBehind()

        destX = ball.x - ChaseConstants.APPROACH_DIST_TO_BALL * \
            cos(radians(destH))
        destY = ball.y - ChaseConstants.APPROACH_DIST_TO_BALL * \
            sin(radians(destH))
        return RobotLocation(destX, destY, destH)

    def getApproachHeadingFromBehind(self):
        ball = self.brain.ball
        aimPoint = KickingHelpers.getShotFarAimPoint(self)
        ballLoc = RobotLocation(ball.x, ball.y, NogginConstants.OPP_GOAL_HEADING)
        ballBearingToGoal = ballLoc.getRelativeBearing(aimPoint)
        return ballBearingToGoal

    def getApproachHeadingFromFront(self):
        ball = self.brain.ball
        my = self.brain.my
        kickDest = KickingHelpers.getShotFarAimPoint(self)
        ballLoc = RobotLocation(ball.x, ball.y, NogginConstants.OPP_GOAL_HEADING)
        ballBearingToKickDest = ballLoc.getRelativeBearing(kickDest)
        if my.y > ball.y:
            destH = ballBearingToKickDest - 90
        else :
            destH = ballBearingToKickDest + 90
        return destH

    def getPenaltyKickingBallDest(self):
        if not self.penaltyMadeFirstKick:
            return Location(NogginConstants.FIELD_WIDTH * 3/4,
                            NogginConstants.FIELD_HEIGHT /4)

        return Location(NogginConstants.OPP_GOAL_MIDPOINT[0],
                        NogginConstants.OPP_GOAL_MIDPOINT[1] )

    def lookPostKick(self):
        tracker = self.brain.tracker
        if self.chosenKick == SweetMoves.RIGHT_SIDE_KICK:
            tracker.lookToDir('left')
        elif self.chosenKick == SweetMoves.LEFT_SIDE_KICK:
            tracker.lookToDir('right')
        else:
            tracker.lookToDir('up')

    def getNextOrbitPos(self):
        relX = -ChaseConstants.ORBIT_OFFSET_DIST * \
               ChaseConstants.COS_ORBIT_STEP_ANGLE + self.brain.ball.relX
        relY =  -ChaseConstants.ORBIT_OFFSET_DIST * \
               ChaseConstants.SIN_ORBIT_STEP_RADIANS + self.brain.ball.relY
        relTheta = ChaseConstants.ORBIT_STEP_ANGLE * 2 + self.brain.ball.bearing
        return RobotLocation(relX, relY, relTheta)

    def shouldMoveAroundBall(self):
        return (self.brain.ball.x < self.brain.my.x
                and (self.brain.my.x - self.brain.ball.x) <
                75.0 )

    def getPointToMoveAroundBall(self):
        ball = self.brain.ball
        x = ball.x

        if ball.y > self.brain.my.y:
            y = self.brain.ball.y - 75.0
            destH = 90.0
        else:
            y = self.brain.ball.y + 75.0
            destH = -90.0

        return RobotLocation(x, y, destH)
