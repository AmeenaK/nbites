''' States for finding our way on the field '''

from .util import MyMath
import NavConstants as constants
from math import fabs

DEBUG = False
# States for the standard spin - walk - spin go to
def spinToWalkHeading(nav):
    """
    Spin to the heading needed to walk to a specific point
    """
    targetH = MyMath.getTargetHeading(nav.brain.my, nav.destX, nav.destY)
    headingDiff = fabs(nav.brain.my.h - targetH)
    newSpinDir = MyMath.getSpinDir(nav.brain.my.h, targetH)

    if nav.firstFrame():
        nav.setSpeed(0,0,0)
        nav.changeSpinDirCounter = 0
        nav.stopSpinToWalkCount = 0
        nav.curSpinDir = newSpinDir

    if newSpinDir != nav.curSpinDir:
        nav.changeSpinDirCounter += 1
    else:
        nav.changeSpinDirCounter = 0

    if nav.changeSpinDirCounter >  constants.CHANGE_SPIN_DIR_THRESH:
        nav.curSpinDir = newSpinDir
        nav.changeSpinDirCounter = 0
        if DEBUG: nav.printf("Switching spin directions my.h " +
                             str(nav.brain.my.h)+
                             " and my target h: " + str(targetH))

    if nav.atHeading(targetH):
        nav.stopSpinToWalkCount += 1
    else :
        nav.stopSpinToWalkCount -= 1
        nav.stopSpinToWalkCount = max(0, nav.stopSpinToWalkCount)

    if nav.stopSpinToWalkCount > constants.GOTO_SURE_THRESH:
        return nav.goLater('walkStraightToPoint')
    if nav.atDestinationCloser():
        return nav.goLater('spinToFinalHeading')

    if not nav.brain.motion.isWalkActive():
        nav.setSpeed(0, nav.curSpinDir * constants.GOTO_SPIN_STRAFE,
                     nav.curSpinDir * constants.GOTO_SPIN_SPEED * \
                         nav.getRotScale(headingDiff))

    return nav.stay()

def walkStraightToPoint(nav):
    """
    State to walk forward w/some turning
    until localization thinks we are close to the point
    Stops if we get there
    If we no longer are heading towards it change to the spin state.
    """
    if nav.firstFrame():
        nav.walkToPointCount = 0
        nav.walkToPointSpinCount = 0

    if nav.atDestinationCloser():
        nav.walkToPointCount += 1
    else :
        nav.walkToPointCount = 0

    if nav.walkToPointCount > constants.GOTO_SURE_THRESH:
        return nav.goLater('spinToFinalHeading')

    targetH = MyMath.getTargetHeading(nav.brain.my, nav.destX, nav.destY)

    if nav.notAtHeading(targetH):
        nav.walkToPointSpinCount += 1
    else :
        nav.walkToPointSpinCount = 0

    if nav.walkToPointSpinCount > constants.GOTO_SURE_THRESH:
        return nav.goLater('spinToWalkHeading')

    bearingToTarget = MyMath.sub180Angle(targetH - nav.brain.my.h)

    sTheta = MyMath.clip(bearingToTarget,
                         -constants.GOTO_STRAIGHT_SPIN_SPEED,
                         constants.GOTO_STRAIGHT_SPIN_SPEED )

    nav.setSpeed(constants.GOTO_FORWARD_SPEED, 0, sTheta)
    return nav.stay()

def spinToFinalHeading(nav):
    """
    Spins until we are facing the final desired heading
    Stops when at heading
    """
    if nav.firstFrame():
        nav.stopSpinToWalkCount = 0

    targetH = nav.destH

    headingDiff = nav.brain.my.h - targetH
    if DEBUG:
        nav.printf("Need to spin to %g, heading diff is %g,heading uncert is %g"
                   % (targetH, headingDiff, nav.brain.my.uncertH))
    spinDir = MyMath.getSpinDir(nav.brain.my.h, targetH)

    strafe = spinDir*constants.GOTO_SPIN_STRAFE
    spin = spinDir*constants.GOTO_SPIN_SPEED*nav.getRotScale(headingDiff)

    if DEBUG: nav.printf("strafe %g, spin %g" % (strafe, spin))

    if nav.atHeading(targetH):
        nav.stopSpinToWalkCount += 1
    else:
        nav.stopSpinToWalkCount = 0

    if nav.stopSpinToWalkCount > constants.CHANGE_SPIN_DIR_THRESH:
        if nav.movingOrtho:
            return nav.goLater('orthoWalkToPoint')
        return nav.goLater('stop')

    nav.setSpeed(0, strafe, spin)
    return nav.stay()

def orthoWalkToPoint(nav):
    """
    State to walk forward until localization thinks we are close to the point
    Stops if we get there
    If we no longer are heading towards it change to the spin state
    """
    my = nav.brain.my
    bearing = MyMath.getRelativeBearing(my.x, my.y, my.h, nav.destX,nav.destY)
    absBearing = fabs(bearing)
    if DEBUG:
        nav.printf('bearing: %g  absBearing: %g  my.x: %g   my.y: %g  my.h: %g'%
                   (bearing, absBearing, my.x, my.y, my.h))
    if DEBUG: nav.printf((' nav.destX: ', nav.destX,
               ' nav.destY: ', nav.destY, ' nav.destH: ', nav.destH))

    if absBearing <= 45:
        return nav.goNow('orthoForward')
    elif absBearing <= 135:
        if bearing < 0:
            return nav.goNow('orthoRightStrafe')
        else:
            return nav.goNow('orthoLeftStrafe')
    elif absBearing <= 180:
            return nav.goNow('orthoBackward')

    return nav.stay()

def orthoForward(nav):

    if nav.firstFrame():
        nav.walkToPointCount = 0
        nav.walkToPointSpinCount = 0
        nav.switchOrthoCount = 0

    if nav.notAtHeading(nav.destH):
        nav.walkToPointSpinCount += 1
        if nav.walkToPointSpinCount > constants.GOTO_SURE_THRESH:
            return nav.goLater('spinToFinalHeading')

    if nav.atDestinationCloser():
        nav.walkToPointCount += 1
        if nav.walkToPointCount > constants.GOTO_SURE_THRESH:
            return nav.goLater('stop')

    nav.setSpeed(constants.GOTO_FORWARD_SPEED,0,0)
    my = nav.brain.my
    bearing = MyMath.getRelativeBearing(my.x, my.y, my.h, nav.destX,nav.destY)

    if -45 <= bearing <= 45:
        return nav.stay()

    nav.switchOrthoCount += 1
    if nav.switchOrthoCount > constants.GOTO_SURE_THRESH:
        return nav.goLater('orthoWalkToPoint')
    return nav.stay()

def orthoBackward(nav):
    if nav.firstFrame():
        nav.walkToPointCount = 0
        nav.walkToPointSpinCount = 0
        nav.switchOrthoCount = 0

    if nav.notAtHeading(nav.destH):
        nav.walkToPointSpinCount += 1
        if nav.walkToPointSpinCount > constants.GOTO_SURE_THRESH:
            return nav.goLater('spinToFinalHeading')

    if nav.atDestinationCloser():
        nav.walkToPointCount += 1
        if nav.walkToPointCount > constants.GOTO_SURE_THRESH:
            return nav.goLater('stop')

    nav.setSpeed(constants.GOTO_BACKWARD_SPEED,0,0)
    my = nav.brain.my
    bearing = MyMath.getRelativeBearing(my.x, my.y, my.h, nav.destX,nav.destY)

    if 135 <= bearing or -135 >= bearing:
        return nav.stay()

    nav.switchOrthoCount += 1
    if nav.switchOrthoCount > constants.GOTO_SURE_THRESH:
        return nav.goLater('orthoWalkToPoint')
    return nav.stay()

def orthoRightStrafe(nav):
    if nav.firstFrame():
        nav.walkToPointCount = 0
        nav.walkToPointSpinCount = 0
        nav.switchOrthoCount = 0

    if nav.notAtHeading(nav.destH):
        nav.walkToPointSpinCount += 1
        if nav.walkToPointSpinCount > constants.GOTO_SURE_THRESH:
            return nav.goLater('spinToFinalHeading')

    if nav.atDestinationCloser():
        nav.walkToPointCount += 1
        if nav.walkToPointCount > constants.GOTO_SURE_THRESH:
            return nav.goLater('stop')

    nav.setSpeed(0, -constants.GOTO_STRAFE_SPEED,0)
    my = nav.brain.my
    bearing = MyMath.getRelativeBearing(my.x, my.y, my.h, nav.destX,nav.destY)

    if -135 <= bearing <= -45:
        return nav.stay()

    nav.switchOrthoCount += 1
    if nav.switchOrthoCount > constants.GOTO_SURE_THRESH:
        return nav.goLater('orthoWalkToPoint')
    return nav.stay()

def orthoLeftStrafe(nav):
    if nav.firstFrame():
        nav.walkToPointCount = 0
        nav.walkToPointSpinCount = 0
        nav.switchOrthoCount = 0

    if nav.notAtHeading(nav.destH):
        nav.walkToPointSpinCount += 1
        if nav.walkToPointSpinCount > constants.GOTO_SURE_THRESH:
            return nav.goLater('spinToFinalHeading')

    if nav.atDestinationCloser():
        nav.walkToPointCount += 1
        if nav.walkToPointCount > constants.GOTO_SURE_THRESH:
            return nav.goLater('stop')

    nav.setSpeed(0, constants.GOTO_STRAFE_SPEED,0)
    my = nav.brain.my
    bearing = MyMath.getRelativeBearing(my.x, my.y, my.h, nav.destX,nav.destY)
    absBearing = abs(bearing)

    if 45 <= bearing <= 135:
        return nav.stay()

    nav.switchOrthoCount += 1
    if nav.switchOrthoCount > constants.GOTO_SURE_THRESH:
        return nav.goLater('orthoWalkToPoint')
    return nav.stay()

def omniWalkToPoint(nav):
    if nav.firstFrame():
        nav.walkToPointCount = 0

    if nav.atDestinationCloser() and nav.atHeading():
        nav.walkToPointCount += 1
        if nav.walkToPointCount > constants.GOTO_SURE_THRESH:
            return nav.goLater('stop')

    my = nav.brain.my
    bearing = MyMath.getRelativeBearing(my.x, my.y, my.h, nav.destX, nav.destY)
    absBearing = abs(bearing)
    verSpeed, horSpeed, spinSpeed = 0.0, 0.0, 0.0

    if bearing != None:
        if absBearing <= 45:
            vertSpeed = constants.GOTO_FORWARD_SPEED
            horSpeed = MyMath.sign(bearing)*constants.GOTO_STRAFE_SPEED*\
                (absBearing/45.0)
        elif absBearing <= 90:
            vertSpeed = constants.GOTO_FORWARD_SPEED*((90 - absBearing)/45.0)
            horSpeed = MyMath.sign(bearing)*constants.GOTO_STRAFE_SPEED
        elif absBearing <= 135:
            vertSpeed = constants.GOTO_BACKWARD_SPEED*((135-absBearing)/45.0)
            horSpeed = MyMath.sign(bearing)*constants.GOTO_STRAFE_SPEED
        elif absBearing <= 180:
            vertSpeed = constants.GOTO_BACKWARD_SPEED
            horSpeed = MyMath.sign(bearing)*constants.GOTO_STRAFE_SPEED*\
                ((180-absBearing)/45.0)

        if nav.oScale == -1:
            nav.oScale = nav.getOScale()
        vertSpeed = nav.oScale*vertSpeed
        horSpeed = nav.oScale*horSpeed

    if nav.destH != None:
        if nav.hScale == -1:
            nav.hScale = nav.getRotScale(my.h - nav.destH)
        spinDir = MyMath.getSpinDir(my.h, nav.destH)
        spinSpeed = constants.GOTO_SPIN_SPEED*spinDir*nav.hScale

    if DEBUG: nav.printf("vertSpeed: %g  horSpeed: %g  spinSpeed: %g" %
               (vertSpeed, horSpeed, spinSpeed))
    nav.setSpeed(vertSpeed, horSpeed, spinSpeed)
    return nav.stay()
# State to be used with standard setSpeed movement
def walking(nav):
    """
    State to be used when setSpeed is called
    """
    if nav.firstFrame() or nav.updatedTrajectory:
        nav.setSpeed(nav.walkX, nav.walkY, nav.walkTheta)
        nav.updatedTrajectory = False

    return nav.stay()
# State to use with the setSteps method
def stepping(nav):
    """
    We use this to go a specified number of steps.
    This is different from walking.
    """
    if nav.firstFrame():
        nav.step(nav.stepX, nav.stepY, nav.stepTheta, nav.numSteps)
    elif not nav.brain.motion.isWalkActive():
        return nav.goNow("stopped")
    return nav.stay()

### Stopping States ###
def stop(nav):
    '''
    Wait until the walk is finished.
    '''
    if nav.firstFrame():
        if nav.brain.motion.isWalkActive():
            nav.setSpeed(0,0,0)
    nav.walkX = nav.walkY = nav.walkTheta = 0


    if not nav.brain.motion.isWalkActive():
        return nav.goNow('stopped')

    return nav.stay()

def stopped(nav):
    nav.walkX = nav.walkY = nav.walkTheta = 0
    return nav.stay()

def orbitPoint(nav):
    if nav.firstFrame():
        nav.setSpeed(0,
                    nav.orbitDir*constants.ORBIT_STRAFE_SPEED,
                    nav.orbitDir*constants.ORBIT_SPIN_SPEED )
    return nav.stay()


def orbitPointThruAngle(nav):
    """
    Circles around a point in front of robot, for a certain angle
    """
    if nav.firstFrame():
        if nav.angleToOrbit < 0:
            nav.orbitDir = constants.ORBIT_LEFT
        else:
            nav.orbitDir = constants.ORBIT_RIGHT

        if fabs(nav.angleToOrbit) <= constants.ORBIT_SMALL_ANGLE:
            sY = constants.ORBIT_STRAFE_SPEED * constants.ORBIT_SMALL_GAIN
            sT = constants.ORBIT_SPIN_SPEED * constants.ORBIT_SMALL_GAIN

        elif fabs(nav.angleToOrbit) <= constants.ORBIT_LARGE_ANGLE:
            sY = constants.ORBIT_STRAFE_SPEED * \
                constants.ORBIT_MID_GAIN
            sT = constants.ORBIT_SPIN_SPEED * \
                constants.ORBIT_MID_GAIN
        else :
            sY = constants.ORBIT_STRAFE_SPEED * \
                constants.ORBIT_LARGE_GAIN
            sT = constants.ORBIT_SPIN_SPEED * \
                constants.ORBIT_LARGE_GAIN

        nav.setSpeed(-0.5, nav.orbitDir*sY, nav.orbitDir*sT)

    #  (frames/second) / (degrees/second) * degrees
    framesToOrbit = fabs((constants.FRAME_RATE / nav.walkTheta) *
                         nav.angleToOrbit)
    if nav.counter >= framesToOrbit:
        return nav.goLater('stop')
    return nav.stay()
