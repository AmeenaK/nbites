
from . import FallStates
from .util import FSA

class FallController(FSA.FSA):
    def __init__(self, brain):
        FSA.FSA.__init__(self,brain)
        self.brain = brain
        #jf- self.setTimeFunction(self.brain.nao.getSimulatedTime)
        self.addStates(FallStates)
        self.currentState = 'notFallen'
        self.setName('FallController')
        self.setPrintStateChanges(True)
        self.stateChangeColor = 'blue'
        self.setPrintFunction(self.brain.out.printf)
        self.standingUp = False
        self.fallCount = 0
        self.FALLEN_THRESH = 72
        self.FALL_COUNT_THRESH = 15

    def run(self):
        # Only try to stand up when playing or localizing in ready
        if (self.brain.gameController.currentState == 'gamePlaying' or
            self.brain.gameController.currentState == 'gameReady' ):
            # Check to see if fallen over
            if (not self.standingUp and self.isFallen() ):
                self.standingUp = True
                self.fallCount = 0
                self.switchTo('fallen')
                #         elif self.brain.guardian.falling:
                #             self.switchTo('falling')
        FSA.FSA.run(self)

    def isFallen(self):
        inertial = self.brain.sensors.inertial
        if ( abs(inertial.angleY) > self.FALLEN_THRESH ):
            self.fallCount += 1
            if self.fallCount > self.FALL_COUNT_THRESH:
                return True
        else:
            self.fallCount = 0
            return False

        return False
