
from math import (fabs, hypot)

from . import PBDefs
from . import PBConstants
from . import Strategies
from .. import NogginConstants
import time

# ANSI terminal color codes
# http://pueblo.sourceforge.net/doc/manual/ansi_color_codes.html
RESET_COLORS_CODE = '\033[0m'
RED_COLOR_CODE = '\033[31m'
GREEN_COLOR_CODE = '\033[32m'
YELLOW_COLOR_CODE = '\033[33m'
BLUE_COLOR_CODE = '\033[34m'
PURPLE_COLOR_CODE = '\033[35m'
CYAN_COLOR_CODE = '\033[36m'

class GoTeam:
    """
    This is the class which controls all of our coordinated behavior system.  
    Should act as a replacement to the old PlayBook monolith approach
    """
    def __init__(self, brain):
        self.brain = brain
        self.printStateChanges = True

        # Info about all of our states
        # Strategies
        self.currentStrategy = 'sInit'
        self.lastStrategy = 'sInit'
        self.lastDiffStrategy = 'sInit'
        self.strategyCounter = 0
        self.strategyStartTime = 0
        self.strategyTime = 0

        # Formations
        self.currentFormation = PBConstants.INIT_FORMATION
        self.lastFormation = PBConstants.INIT_FORMATION
        self.lastDiffFormation = PBConstants.INIT_FORMATION
        self.formationCounter = 0
        self.formationStartTime = 0
        self.formationTime = 0

        # Roles
        self.currentRole = PBConstants.INIT_ROLE
        self.lastRole = PBConstants.INIT_ROLE
        self.lastDiffRole = PBConstants.INIT_ROLE
        self.roleCounter = 0
        self.roleStartTime = 0
        self.roleTime = 0

        # SubRoles
        self.currentSubRole = PBConstants.INIT_SUB_ROLE
        self.lastSubRole = PBConstants.INIT_SUB_ROLE
        self.lastDiffSubRole = PBConstants.INIT_SUB_ROLE
        self.subRoleCounter = 0
        self.subRoleStartTime = 0
        self.subRoleTime = 0

        self.subRoleOnDeck = PBConstants.INIT_SUB_ROLE
        self.lastSubRoleOnDeck = PBConstants.INIT_SUB_ROLE
        self.subRoleOnDeckCount = 0

        self.subRole = self.currentSubRole
        self.role = self.currentRole
        self.formation = self.currentFormation
        self.strategy = self.currentStrategy

        # Information about teammates
        self.teammates = []
        for i in xrange(PBConstants.NUM_TEAM_PLAYERS):
            mate = PBDefs.Teammate(brain)
            mate.playerNumber = i + 1
            self.teammates.append(mate)
       
        self.position = []
        self.me = self.teammates[self.brain.my.playerNumber - 1]
        self.me.playerNumber = self.brain.my.playerNumber
        self.inactiveMates = [] # inactive field players only
        self.numInactiveMates = 0
        self.kickoffFormation = 0
        self.timeSinceCaptureChase = 0

    def run(self):
        """
        We run this each frame to get the latest info
        """
        if self.brain.gameController.currentState != 'gamePenalized':
            self.aPrioriTeammateUpdate()
        # We will always return a strategy
        self.currentStrategy, self.currentFormation, self.currentRole, \
            self.currentSubRole, self.position = self.strategize()
        # Update all of our new infos
        self.updateStateInfo()
        self.aPosterioriTeammateUpdate()

    def strategize(self):
        """
        Picks the strategy to run and returns all sorts of infos
        """
        if self.brain.gameController.currentState == 'gameInitial' or\
            self.brain.gameController.currentState == 'gameFinished':
            return ('sInit', PBConstants.INIT_FORMATION, PBConstants.INIT_ROLE,
                    PBConstants.INIT_SUB_ROLE, [0,0] )
        elif self.brain.gameController.currentState == 'gamePenalized':
            return ('sInit', PBConstants.PENALTY_FORMATION, PBConstants.PENALTY_ROLE,
                    PBConstants.PENALTY_SUB_ROLE, [0,0] )
        # First we check for testing stuff
        elif PBConstants.TEST_DEFENDER:
            return Strategies.sTestDefender(self)
        elif PBConstants.TEST_OFFENDER:
            return Strategies.sTestOffender(self)
        elif PBConstants.TEST_CHASER:
            return Strategies.sTestChaser(self)

        # Now we look at shorthanded strategies
        elif self.numInactiveMates == 1:
            return Strategies.sOneDown(self)
        elif self.numInactiveMates == 2:
            return Strategies.sNoFieldPlayers(self)
        # Here we have the strategy stuff
        return Strategies.sSpread(self)

    def updateStateInfo(self):
        """
        Update information specific to the coordinated behaviors
        """
        # Update our counters!
        # Update Strategy Memory
        if self.lastStrategy != self.currentStrategy:
            if self.printStateChanges:
                self.printf("Strategy switched to "+
                            self.currentStrategy)
            self.strategyCounter = 0
            self.lastDiffStrategy = self.lastStrategy
            self.strategyStartTime = self.getTime()
            self.strategyTime = 0
        else:
            self.strategyCounter += 1
            self.strategyTime = self.getTime() - self.strategyStartTime

        # Update Formations memory
        if self.lastFormation != self.currentFormation:
            if self.printStateChanges:
                self.printf("Formation switched to "+
                            PBConstants.FORMATIONS[self.currentFormation])
            self.formationCounter = 0
            self.lastDiffFormation = self.lastFormation
            self.formationStartTime = self.getTime()
            self.formationTime = 0
        else:
            self.formationCounter += 1
            self.formationTime = self.getTime() - self.formationStartTime

        # Update memory of roles
        if self.lastRole != self.currentRole:
            if self.printStateChanges:
                self.printf("Role switched to "+
                            PBConstants.ROLES[self.currentRole])
            self.roleCounter = 0
            self.lastDiffRole = self.lastRole
            self.roleStartTime = self.getTime()
            self.roleTime = 0
            self.subRoleOnDeck = self.currentSubRole
            self.subRoleOnDeckCounter = 0
        else:
            self.roleCounter += 1
            self.roleTime = self.getTime() - self.roleStartTime

        # We buffer the subRole switching by making sure that a subrole is 
        # returned for multiple frames in a row
        if (self.subRoleOnDeck and 
            self.subRoleOnDeck != self.currentSubRole):
            # if the sub role on deck is the same as the last, increment
            if self.subRoleOnDeck == self.lastSubRoleOnDeck:
                self.subRoleOnDeckCounter += 1
            # otherwise our idea of subRole has changed, null counter
            else:
                self.subRoleOnDeckCounter = 0

            # if we are sure that we want to switch into this sub role
            if (self.subRoleOnDeckCounter > 
                PBConstants.SUB_ROLE_SWITCH_BUFFER):
                #self.currentSubRole = self.subRoleOnDeck
                self.subRoleOnDeckCounter = 0

        # SubRoles
        if self.lastSubRole != self.currentSubRole:
            if self.printStateChanges:
                try:
                    self.printf("SubRole switched to "+
                                PBConstants.SUB_ROLES[self.currentSubRole])
                except:
                    raise ValueError("Incorrect currentSubRole:" + 
                                     str(self.currentSubRole))
            self.roleCounter = 0
            self.lastDiffSubRole = self.lastSubRole
            self.subRoleStartTime = self.getTime()
            self.subRoleTime = 0
        else:
            self.subRoleCounter += 1
            self.subRoleTime = self.getTime() - self.subRoleStartTime

        # Get ready for the next frame
        self.lastStrategy = self.currentStrategy
        self.lastFormation = self.currentFormation
        self.lastRole = self.currentRole
        self.lastSubRole = self.currentSubRole
        self.lastSubRoleOnDeck = self.subRoleOnDeck

        # Robots are funnnn.... :(
        self.strategy = self.currentStrategy
        self.formation = self.currentFormation
        self.role = self.currentRole
        self.subRole = self.currentSubRole

    ######################################################
    ############       Role Switching Stuff     ##########
    ######################################################
    def determineChaser(self):
        '''return the player number of the chaser'''

        chaser_mate = self.teammates[self.brain.my.playerNumber-1]

        if PBConstants.DEBUG_DET_CHASER:
            print "chaser det: me == #", self.brain.my.playerNumber
        # scroll through the teammates
        for i, mate in enumerate(self.teammates):
            if PBConstants.DEBUG_DET_CHASER:
                print "\t mate #", i+1,
            # TEAMMATE SANITY CHECKS
            # don't check goalie out
            # don't check out penalized or turned-off dogs
            if (mate.playerNumber == PBConstants.GOALIE_NUMBER or  
                mate.inactive or 
                mate.playerNumber == self.brain.my.playerNumber or 
                fabs(mate.ballY - self.brain.ball.y) > 150.0):
                if PBConstants.DEBUG_DET_CHASER:
                    print "\t inactive or goalie or me"
                continue

            # if the player has got the ball, return his number
            if (mate.grabbing or
                mate.dribbling or
                mate.kicking):
                if PBConstants.DEBUG_DET_CHASER:
                    print "\t grabbing"
                return mate
            # if both robots see the ball use visual distances to ball
            if ((mate.ballDist > 0 and chaser_mate.ballDist > 0) and
                (mate.ballDist < chaser_mate.ballDist)):
                chaser_mate = mate 
            # use loc distances if both don't have a visual ball
            elif mate.ballLocDist < chaser_mate.ballLocDist:
                chaser_mate = mate
            
            if PBConstants.DEBUG_DET_CHASER:
                print ("\t #%d @ %g >= #%d @ %g" % 
                       (mate.playerNumber, mate.ballDist, 
                        chaser_mate.playerNumber, 
                        chaser_mate.ballDist))

        if PBConstants.DEBUG_DET_CHASER:
            print "\t ---- MATE %g WINS" % (chaser_mate.playerNumber)
        # returns teammate instance (could be mine)
        return chaser_mate

    def getLeastWeightPosition(self,positions, mates = None):
        """
        Gets the position for the dog such that the distance all dogs have
        to move is the least possible
        """
        # if there is only one position return the position
        if len(positions) == 1 or mates == None:
            return positions[0]

        # if we have two positions only two possibilites of positions
        #TODO: tie-breaking?
        else: # len(positions) == 2
            myDist1 = hypot(positions[0][0] - self.brain.my.x,
                            positions[0][1] - self.brain.my.y)
            myDist2 = hypot(positions[1][0] - self.brain.my.x,
                            positions[1][1] - self.brain.my.y)
            mateDist1 = hypot(positions[0][0] - mates.x,
                              positions[0][1] - mates.y)
            mateDist2 = hypot(positions[1][0] - mates.x,
                              positions[1][1] - mates.y)

            if myDist1 + mateDist2 == myDist2 + mateDist1:
                if self.me.playerNumber > mates.playerNumber:
                    return positions[0]
                else:
                    return positions[1]
            elif myDist1 + mateDist2 < myDist2 + mateDist1:
                return positions[0]
            else:
                return positions[1]

    ######################################################
    ############       Teammate Stuff     ################
    ######################################################
        
    def aPrioriTeammateUpdate(self):
        '''
        Here we update information about teammates before running a new frame
        '''
        # Change which wing is forward based on the opponents score
        # self.kickoffFormation = (self.brain.gameController.theirTeam.teamScore) % 2

        # update my own information for role switching
        self.me.updateMe()

        # loop through teammates
        for mate in self.teammates:
            if (mate.isPenalized()): # or mate.isDead()
                #reset to true when we get a new packet from mate
                mate.inactive = True
            if (mate.ballDist > 0):
                self.brain.ball.reportBallSeen()
        self.inactiveMates = self.getInactiveFieldPlayers()
        self.numInactiveMates = len(self.inactiveMates)

    def aPosterioriTeammateUpdate(self):
        """
        Here are updates to teammates which occur after running before 
        exiting the frame
        """
        pass

    def getInactiveFieldPlayers(self):
        '''cycles through teammate objects and returns teammates
        that are 'dead'. ignores myself'''
        inactive_teammates = []
        for i,mate in enumerate(self.teammates):
            if (mate.inactive and mate.playerNumber != 
                self.brain.my.playerNumber and 
                mate.playerNumber != PBConstants.GOALIE_NUMBER):
                self.printf(mate.playerNumber)
                inactive_teammates.append(mate)
        return inactive_teammates

    def highestActivePlayerNumber(self):
        '''returns true if the player is the highest active player number'''
        activeMate = self.getOtherActiveTeammate()
        if activeMate.playerNumber > self.me.playerNumber:
            return False
        return True

    def teammateHasBall(self):
        '''returns True if any mate has the ball'''
        for i,mate in enumerate(self.teammates):
            if (mate.inactive or 
                mate.playerNumber == self.me.playerNumber):
                continue
            elif (mate.hasBall()):
                return True
        return False

    def getOtherActiveTeammate(self):
        '''this returns the teammate instance of an active teammate that isn't 
        you.'''
        if self.me.playerNumber == 3:
            return self.teammates[1] #returns player 2
        else:
            return self.teammates[2] #returns player 3

    def getOtherActiveTeammates(self):
        '''
        figure out who the other active teammates are
        '''
        mates = []
        for mate in self.teammates:
            if (not mate.inactive and mate.playerNumber != 
                PBConstants.GOALIE_NUMBER
                and mate.playerNumber != self.me.playerNumber):
                mates.append(mate)
        return mates

    def isGoalieInactive(self):
        '''
        Determines if the goalie is inactive
        '''
        return (self.teammates[0].inactive)

    def isGoalie(self):
        return self.brain.my.playerNumber == 1

    def reset(self):
        '''resets all information stored from teammates'''
        for i,mate in enumerate(self.teammates):
            mate.reset()

    def update(self,packet):
        '''public method called by Brain.py to update a teammates' info
        with a new packet'''
        self.teammates[packet.playerNumber-1].update(packet)

    ######################################################
    ############   Strategy Decision Stuff     ###########
    ######################################################
    def shouldUseDubD(self):
        return ((self.brain.ball.y > NogginConstants.MY_GOALBOX_BOTTOM_Y + 5. and
                 self.brain.ball.y < NogginConstants.MY_GOALBOX_TOP_Y -5.  and
                 self.brain.ball.x < NogginConstants.MY_GOALBOX_RIGHT_X - 5.) or
                (self.brain.ball.y > NogginConstants.MY_GOALBOX_TOP_Y - 5. and
                 self.brain.ball.y < NogginConstants.MY_GOALBOX_BOTTOM_Y + 5. and
                 self.brain.ball.x < NogginConstants.MY_GOALBOX_RIGHT_X + 5. and
                 self.teammates[0].calledRole == PBConstants.CHASER))

    def ballInMyGoalBox(self):
        '''
        returns True if estimate of ball (x,y) lies in my goal box
        -includes all y values below top of goalbox 
        (so inside the goal is included)
        '''
        return (self.brain.ball.y > NogginConstants.MY_GOALBOX_BOTTOM_Y and
                self.brain.ball.x < NogginConstants.MY_GOALBOX_RIGHT_X and
                self.brain.ball.y < NogginConstants.MY_GOALBOX_TOP_Y)

    def getPointBetweenBallAndGoal(self,dist_from_ball):
        '''returns defensive position between ball (x,y) and goal (x,y)
        at <dist_from_ball> centimeters away from ball'''
        delta_y = self.brain.ball.y - NogginConstants.MY_GOALBOX_MIDDLE_Y

        delta_x = self.brain.ball.x - NogginConstants.MY_GOALBOX_LEFT_X

        # don't divide by 0
        if delta_x == 0:
            delta_x = 0.1
        if delta_y == 0:
            delta_y = 0.1

        pos_x = self.brain.ball.x - (dist_from_ball/
                                     hypot(delta_x,delta_y))*delta_x
        pos_y = self.brain.ball.y - (dist_from_ball/
                                     hypot(delta_x,delta_y))*delta_y
        if pos_x > PBConstants.STOPPER_MAX_X:
            pos_x = PBConstants.STOPPER_MAX_X
            pos_y = (NogginConstants.MY_GOALBOX_MIDDLE_Y + delta_x / delta_y *
                     (PBConstants.STOPPER_MAX_X - 
                      NogginConstants.MY_GOALBOX_LEFT_X))

        return pos_x,pos_y

    def noCalledChaser(self):
        """
        Returns true if no one is chasing and they are not searching
        """
        # If everyone else is out, let's not go for the ball
        if len(self.getInactiveFieldPlayers()) == \
                PBConstants.NUM_TEAM_PLAYERS - 1.:
            return False

        if self.brain.gameController.currentState == 'gameReady' or\
                self.brain.gameController.currentState =='gameSet':
            return False

        for mate in self.teammates:
            if (not mate.inactive and (mate.calledRole == PBConstants.CHASER
                                       or mate.calledRole == 
                                       PBConstants.SEARCHER)):
                return False
        return True

################################################################################
#####################     Utility Functions      ###############################
################################################################################

    def getTime(self):
        return time.time()

    def printf(self,outputString, printingColor='purple'):
        '''FSA print function that allows colors to be specified'''
        if printingColor == 'red':
            self.brain.out.printf(RED_COLOR_CODE + str(outputString) +\
                RESET_COLORS_CODE)
        elif printingColor == 'blue':
            self.brain.out.printf(BLUE_COLOR_CODE + str(outputString) +\
                RESET_COLORS_CODE)
        elif printingColor == 'yellow':
            self.brain.out.printf(YELLOW_COLOR_CODE + str(outputString) +\
                RESET_COLORS_CODE)
        elif printingColor == 'cyan':
            self.brain.out.printf(CYAN_COLOR_CODE + str(outputString) +\
                RESET_COLORS_CODE)
        elif printingColor == 'purple':
            self.brain.out.printf(PURPLE_COLOR_CODE + str(outputString) +\
                RESET_COLORS_CODE)
        else:
            self.brain.out.printf(str(outputString))
