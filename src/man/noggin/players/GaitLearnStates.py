import math

import man.motion as motion
import man.motion.SweetMoves as SweetMoves
import man.motion.RobotGaits as RobotGaits
import man.motion.MotionConstants as MotionConstants
from ..WebotsConfig import WEBOTS_ACTIVE
import man.noggin.util.PSO as PSO
from man.motion.gaits.GaitLearnBoundaries import gaitMins, gaitMaxs, gaitToArray, arrayToGaitTuple
from os.path import isfile

try:
   import cPickle as pickle
except:
   import pickle

PSO_STATE_FILE = "PSO_pGaitLearner.pickle"

def gamePlaying(player):
    print "In the players version of game controller state (overridden)"
    if player.firstFrame():
        player.gainsOn()
        player.brain.tracker.trackBall()

        startPSO(player)

    return player.goLater('stopandchangegait')

if WEBOTS_ACTIVE:
    gameInitial=gamePlaying
    print "Webots is active!!!!"
else:
    print "Webots is in-active!!!!"

def walkstraightstop(player):
    # unstable walks that do not fall have accelerometer variance
    # on the order of 1.5-3, stable walks are < 1
    X_STABILITY_WEIGHT = 100
    Y_STABILITY_WEIGHT = 100

    if player.firstFrame():
        # TODO :: make this more flexible
        setWalkVector(player, 15,0,0)
        player.brain.stability.resetData()

    if player.counter == 800:
        # we optimize towards high stability
        frames_stood = player.counter

        stability = frames_stood - X_STABILITY_WEIGHT*player.brain.stability.getStability_X() \
            - Y_STABILITY_WEIGHT*player.brain.stability.getStability_Y()

        player.swarm.getCurrParticle().setStability(stability)
        player.swarm.tickCurrParticle()

        return player.goNow('stopandchangegait')

    if player.brain.stability.isWBFallen():
        print "(GaitLearning):: we've fallen down!"
        player.swarm.getCurrParticle().setStability(player.counter)
        player.swarm.tickCurrParticle()
        return player.goLater('standuplearn')

    return player.stay()

def stopwalking(player):
    ''' Do nothing'''
    if player.firstFrame():
        setWalkVector(player, 0,0,0)

    if player.counter == 200:
        return player.goLater('walkstraightstop')

    return player.stay()

def stopandchangegait(player):
    '''Save the PSO state, set new gait
    and start walking again'''

    if player.firstFrame():
        setWalkVector(player, 0,0,0)

        savePSO(player)

        # set gait from new particle
        gaitTuple = arrayToGaitTuple(player.swarm.getNextParticle().getPosition())

        newGait = motion.GaitCommand(gaitTuple[0],
                                     gaitTuple[1],
                                     gaitTuple[2],
                                     gaitTuple[3],
                                     gaitTuple[4],
                                     gaitTuple[5],
                                     gaitTuple[6],
                                     gaitTuple[7])

        player.brain.CoA.setRobotDynamicGait(player.brain.motion, newGait)

        #print "gait in tuple form:"
        #for tuple in gaitTuple:
        #    print tuple

    if player.counter == 30:
        return player.goLater('walkstraightstop')

    return player.stay()

def standup(player):
    if player.firstFrame():
        player.executeMove(SweetMoves.ZERO_POS)
    return player.stay()

def standuplearn(player):
    if player.firstFrame():
        player.executeMove(SweetMoves.STAND_UP_FRONT)

    return player.goLater('stopandchangegait')

def sitdown(player):
    if player.firstFrame():
        player.executeMove(SweetMoves.SIT_POS)

    elif player.brain.motion.isBodyActive():
        return player.goLater("printloc")

    return player.stay()

def shutoffgains(player):
    if player.firstFrame():
        shutoff = motion.StiffnessCommand(0.0)
        player.brain.motion.sendStiffness(shutoff)

    return player.goLater('nothing')

def printloc(player):
    if player.firstFrame():
        player.printf("Loc (X,Y,H) (%g,%g,%g"%
                      (player.brain.my.x,
                       player.brain.my.y,
                       player.brain.my.h))
    return player.stay()

def startPSO(player):
    if isfile(PSO_STATE_FILE):
        loadPSO(player)
    else:
        print "Initializing new PSO"
        player.swarm = PSO.Swarm(25, 44, gaitToArray(gaitMins), gaitToArray(gaitMaxs))

def savePSO(player):
    print "Saving PSO state to: ", PSO_STATE_FILE
    f = open(PSO_STATE_FILE, 'w')
    pickle.dump(player.swarm, f)
    f.close()

def loadPSO(player):
    print "Loading PSO state from: ", PSO_STATE_FILE
    f = open(PSO_STATE_FILE, 'r')
    player.swarm = pickle.load(f)
    f.close()

def setWalkVector(player, x, y, theta):
    """
    Use this guy because all the wrappings through Nav tend
    to reset our gait parameters
    """
    walk = motion.WalkCommand(x=x,y=y,theta=theta)
    player.brain.motion.setNextWalkCommand(walk)
