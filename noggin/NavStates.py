''' States for finding our way on the field '''

from .util import MyMath

FAST_GAIT = "fastGait"
NORMAL_GAIT = "normalGait"
GOTO_FORWARD_SPEED = 4
GOTO_BACKWARD_SPEED = -2
GOTO_STRAFE_SPEED = 2
WAIT_BETWEEN_MOVES = 0
GOTO_SPIN_SPEED = 10
GOTO_SPIN_STRAFE = 0
GOTO_SURE_THRESH = 5

CHANGE_SPIN_DIR_THRESH = 3

DEBUG = False
# States for the standard spin - walk - spin go to
def spinToWalkHeading(nav):
    """
    Spin to the heading needed to walk to a specific point
    """
    targetH = MyMath.getTargetHeading(nav.brain.my, nav.destX, nav.destY)
    headingDiff = abs(nav.brain.my.h - targetH)
    newSpinDir = MyMath.getSpinDir(nav.brain.my, targetH)


    if nav.firstFrame():
        nav.setSpeed(0,0,0)
        nav.stopSpinToWalkCount = 0
        nav.noWalkSet = True
        nav.curSpinDir = newSpinDir
        nav.changeSpinDirCounter = 0

    if nav.noWalkSet and nav.brain.motion.isWalkActive():
        if DEBUG: nav.printf("Waiting for walk to stop")
        return nav.stay()


#     nav.printf("Target heading is " + str(targetH))
#     nav.printf("Current heading is " + str(nav.brain.my.h))

    spinDir = nav.curSpinDir
    if newSpinDir != nav.curSpinDir:
        nav.changeSpinDirCounter += 1
    else:
        nav.changeSpinDirCounter -= 1
        nav.changeSpinDirCounter = max(0,nav.changeSpinDirCounter)

    if nav.changeSpinDirCounter >  CHANGE_SPIN_DIR_THRESH:
        spinDir = newSpinDir
        nav.curSpinDir = spinDir
        nav.changeSpinDirCounter = 0
        if DEBUG: nav.printf("Switching spin directions my.h " + str(nav.brain.my.h)+
                   " and my target h: " + str(targetH))
    else:
        spinDir = nav.curSpinDir

    if spinDir == None:
        nav.printf("Spindir is none: nav.curSpinDir" +str(nav.curSpinDir) +
                   " newSpinDir is " + str(newSpinDir))
    nav.setSpeed(0, spinDir * GOTO_SPIN_STRAFE,
                 spinDir * GOTO_SPIN_SPEED*nav.getRotScale(headingDiff))
    nav.noWalkSet = False

    if nav.atHeading(targetH):
        nav.stopSpinToWalkCount += 1

    if nav.stopSpinToWalkCount > GOTO_SURE_THRESH:
        return nav.goLater('walkToPoint')

    return nav.stay()

def walkToPoint(nav):
    """
    State to walk forward until localization thinks we are close to the point
    Stops if we get there
    If we no longer are heading towards it change to the spin state
    """
    if nav.firstFrame():
        nav.setSpeed(0,0,0)
        nav.walkToPointCount = 0
        nav.walkToPointSpinCount = 0
        nav.noWalkSet  = True

    if nav.noWalkSet and nav.brain.motion.isWalkActive():
        if DEBUG: nav.printf("Waiting for walk to stop")
        return nav.stay()

    nav.setSpeed(GOTO_FORWARD_SPEED, 0, 0)
    nav.noWalkSet = False


    if nav.atDestination():
        nav.walkToPointCount += 1

    if nav.walkToPointCount > GOTO_SURE_THRESH:
        if nav.destH is None:
#             nav.printf("Stopping at position (" + str(nav.brain.my.x) +
#                        ", " + str(nav.brain.my.y) + ") going to (" + str(nav.destX)
#                        + ", " + str(nav.destY) + ")")
            return nav.goLater('stop')
        else:
            return nav.goLater('spinToFinalHeading')

    targetH = MyMath.getTargetHeading(nav.brain.my, nav.destX, nav.destY)

    if nav.notAtHeading(targetH):
        nav.walkToPointSpinCount += 1

    if nav.walkToPointSpinCount > GOTO_SURE_THRESH:
        return nav.goLater('spinToWalkHeading')

    return nav.stay()

def spinToFinalHeading(nav):
    """
    Spins until we are facing the final desired heading
    Stops when at heading
    """
    if nav.firstFrame():
        nav.noWalkSet  = True
        nav.stopSpinToWalkCount = 0

    targetH = nav.destH#MyMath.getTargetHeading(nav.brain.my, nav.destX, nav.destY)
    headingDiff = abs(nav.brain.my.h - targetH)
    if DEBUG: nav.printf("Need to spin to %g, heading diff is %g, heading uncert is %g" %
               (targetH, headingDiff, nav.brain.my.uncertH))
    spinDir = MyMath.getSpinDir(nav.brain.my, targetH)

    strafe = spinDir*GOTO_SPIN_STRAFE
    spin = spinDir*GOTO_SPIN_SPEED*nav.getRotScale(headingDiff)
    if DEBUG: nav.printf("strafe %g, spin %g" % (strafe, spin))
    nav.setSpeed(0, strafe, spin)

    nav.noWalkSet = False
    if nav.atHeading(targetH):
        nav.stopSpinToWalkCount += 1
        if nav.stopSpinToWalkCount > CHANGE_SPIN_DIR_THRESH:
            if nav.movingOrtho:
                return nav.goLater('orthoWalkToPoint')
            return nav.goLater('stop')
    return nav.stay()

def orthoWalkToPoint(nav):
    """
    State to walk forward until localization thinks we are close to the point
    Stops if we get there
    If we no longer are heading towards it change to the spin state
    """

    my = nav.brain.my
    bearing = MyMath.getRelativeBearing(my.x, my.y, my.h, nav.destX,nav.destY)
    absBearing = abs(bearing)
    if DEBUG: nav.printf('bearing: %g  absBearing: %g  my.x: %g   my.y: %g  my.h: %g' %
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
        nav.noWalkSet  = True

    if nav.notAtHeading(nav.destH):
        nav.walkToPointSpinCount += 1
        if nav.walkToPointSpinCount > GOTO_SURE_THRESH:
            return nav.goLater('spinToFinalHeading')

    if nav.atDestinationCloser():
        nav.walkToPointCount += 1
        if nav.walkToPointCount > GOTO_SURE_THRESH:
            return nav.goLater('stop')

    nav.setSpeed(GOTO_FORWARD_SPEED,0,0)
    my = nav.brain.my
    bearing = MyMath.getRelativeBearing(my.x, my.y, my.h, nav.destX,nav.destY)

    if -45 <= bearing <= 45:
        return nav.stay()

    nav.switchOrthoCount += 1
    if nav.switchOrthoCount > GOTO_SURE_THRESH:
        return nav.goLater('orthoWalkToPoint')
    return nav.stay()

def orthoBackward(nav):
    if nav.firstFrame():
        nav.walkToPointCount = 0
        nav.walkToPointSpinCount = 0
        nav.switchOrthoCount = 0
        nav.noWalkSet  = True

    if nav.notAtHeading(nav.destH):
        nav.walkToPointSpinCount += 1
        if nav.walkToPointSpinCount > GOTO_SURE_THRESH:
            return nav.goLater('spinToFinalHeading')

    if nav.atDestinationCloser():
        nav.walkToPointCount += 1
        if nav.walkToPointCount > GOTO_SURE_THRESH:
            return nav.goLater('stop')

    nav.setSpeed(GOTO_BACKWARD_SPEED,0,0)
    my = nav.brain.my
    bearing = MyMath.getRelativeBearing(my.x, my.y, my.h, nav.destX,nav.destY)

    if 135 <= bearing or -135 >= bearing:
        return nav.stay()

    nav.switchOrthoCount += 1
    if nav.switchOrthoCount > GOTO_SURE_THRESH:
        return nav.goLater('orthoWalkToPoint')
    return nav.stay()

def orthoRightStrafe(nav):
    if nav.firstFrame():
        nav.walkToPointCount = 0
        nav.walkToPointSpinCount = 0
        nav.switchOrthoCount = 0
        nav.noWalkSet  = True

    if nav.notAtHeading(nav.destH):
        nav.walkToPointSpinCount += 1
        if nav.walkToPointSpinCount > GOTO_SURE_THRESH:
            return nav.goLater('spinToFinalHeading')

    if nav.atDestinationCloser():
        nav.walkToPointCount += 1
        if nav.walkToPointCount > GOTO_SURE_THRESH:
            return nav.goLater('stop')

    nav.setSpeed(0, -GOTO_STRAFE_SPEED,0)
    my = nav.brain.my
    bearing = MyMath.getRelativeBearing(my.x, my.y, my.h, nav.destX,nav.destY)

    if -135 <= bearing <= -45:
        return nav.stay()

    nav.switchOrthoCount += 1
    if nav.switchOrthoCount > GOTO_SURE_THRESH:
        return nav.goLater('orthoWalkToPoint')
    return nav.stay()

def orthoLeftStrafe(nav):
    if nav.firstFrame():
        nav.walkToPointCount = 0
        nav.walkToPointSpinCount = 0
        nav.switchOrthoCount = 0
        nav.noWalkSet  = True

    if nav.notAtHeading(nav.destH):
        nav.walkToPointSpinCount += 1
        if nav.walkToPointSpinCount > GOTO_SURE_THRESH:
            return nav.goLater('spinToFinalHeading')

    if nav.atDestinationCloser():
        nav.walkToPointCount += 1
        if nav.walkToPointCount > GOTO_SURE_THRESH:
            return nav.goLater('stop')

    nav.setSpeed(0, GOTO_STRAFE_SPEED,0)
    my = nav.brain.my
    bearing = MyMath.getRelativeBearing(my.x, my.y, my.h, nav.destX,nav.destY)
    absBearing = abs(bearing)

    if 45 <= bearing <= 135:
        return nav.stay()

    nav.switchOrthoCount += 1
    if nav.switchOrthoCount > GOTO_SURE_THRESH:
        return nav.goLater('orthoWalkToPoint')
    return nav.stay()

# State to be used with standard setSpeed movement
def walking(nav):
    """
    State to be used when setSpeed is called
    """
    if nav.firstFrame() or nav.updatedTrajectory:
        #HACK nav.brain.CoA.setRobotGait(nav.brain.motion)
#         nav.printf("New walk is (" + str(nav.walkX) + ", " +
#                    str(nav.walkY) + ", " + str(nav.walkTheta) + ")")
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
        return nav.goLater("stopped")
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
