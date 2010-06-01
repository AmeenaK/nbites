import man.motion as motion
from man.noggin.util import MyMath

def setSpeed(nav, x, y, theta):
    """
    Wrapper method to easily change the walk vector of the robot
    """
    walk = motion.WalkCommand(x=x,y=y,theta=theta)
    nav.brain.motion.setNextWalkCommand(walk)

    nav.walkX, nav.walkY, nav.walkTheta = x, y, theta
    nav.curSpinDir = MyMath.sign(theta)


def step(nav, x, y, theta, numSteps):
    """
    Wrapper method to easily change the walk vector of the robot
    """
    steps = motion.StepCommand(x=x,y=y,theta=theta,numSteps=numSteps)
    nav.brain.motion.sendStepCommand(steps)

    nav.walkX, nav.walkY, nav.walkTheta = x, y, theta
    nav.curSpinDir = MyMath.sign(theta)

def executeMove(motionInst, sweetMove):
    """
    Method to enqueue a SweetMove
    Can either take in a head move or a body command
    (see SweetMove files for descriptions of command tuples)
    """

    for position in sweetMove:
        if len(position) == 7:
            move = motion.BodyJointCommand(position[4], #time
                                           position[0], #larm
                                           position[1], #lleg
                                           position[2], #rleg
                                           position[3], #rarm
                                           position[6], # Chain Stiffnesses
                                           position[5], #interpolation type
                                           )

        elif len(position) == 5:
            move = motion.BodyJointCommand(position[2], # time
                                           position[0], # chainID
                                           position[1], # chain angles
                                           position[4], # chain stiffnesses
                                           position[3], # interpolation type
                                           )

        else:
            print("What kind of sweet ass-Move is this?")

        motionInst.enqueue(move)
