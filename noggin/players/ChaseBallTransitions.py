import man.motion.SweetMoves as SweetMoves
import ChaseBallConstants as constants


def shouldTurnToBall_FoundBall(player):
    if player.brain.ball.framesOn > constants.FRAMES_ON_THRESH:
        return True
    return False

def shouldTurnToBall_ApproachBall(player):
    ball = player.brain.ball
    if (ball.on and
        abs(ball.locBearing) > constants.BALL_BEARING_THRESH):
        return True
    return False

def shouldCantFindBall(player):
    if player.stateTime >= constants.GIVE_UP_THRESH:
        return True
    return False

def shouldSpinFindBall(player):
    if player.stateTime >= SweetMoves.getMoveTime(SweetMoves.SCAN_BALL):
        return True
    return False

def shouldSpinFindBallPosition(player):
    if player.brain.ball.framesOff >= 40:
        return True
    return False

def shouldScanFindBall(player):
    if player.brain.ball.framesOff > constants.FRAMES_OFF_THRESH:
        return True
    return False

def shouldApproachBall(player):
    ball = player.brain.ball
    if ( ball.on and
         abs(ball.locBearing) < constants.BALL_BEARING_THRESH ):
        return True
    return False

def shouldApproachBallClose(player):
    ball = player.brain.ball
    if ball.locDist < constants.BALL_CLOSE_DIST_THRESH and \
            abs(ball.locBearing) < constants.BALL_CLOSE_APPROACH_BEARING_THRESH and\
            ball.on :
        return True
    return False

def shouldTurnToBallClose(player):
    ball = player.brain.ball
    if ( ball.on and
         abs(ball.locBearing) > constants.BALL_CLOSE_TURN_BEARING_THRESH and
         ball.locDist < constants.BALL_CLOSE_DIST_THRESH):
        return True

def shouldPositionForKick(player):
    ball = player.brain.ball
    if (ball.on and
        abs(ball.locBearing) < constants.BALL_POS_KICK_BEARING_THRESH and
        ball.locDist < constants.BALL_POS_KICK_DIST_THRESH ):
        return True
    return False

def shouldApproachForKick(player):
    ball = player.brain.ball
    if (ball.on and
        abs(ball.locBearing) < constants.BALL_POS_KICK_BEARING_THRESH and
        ball.locDist > constants.BALL_POS_KICK_DIST_THRESH + 10 ):
        return True
    return False

def shouldTurnForKick(player):
    ball = player.brain.ball
    if ( ball.on and
          abs(ball.locBearing) > constants.BALL_TURN_KICK_BEARING_THRESH and
         ball.locDist < constants.BALL_POS_KICK_DIST_THRESH ):
        return True
    return False

def shouldKick(player):
    ball = player.brain.ball
    if (ball.on and
        ball.locRelY > constants.BALL_KICK_LEFT_Y_R and
        ball.locRelY < constants.BALL_KICK_LEFT_Y_L and
        ball.locRelX > constants.BALL_KICK_LEFT_X_CLOSE and
        ball.locRelX < constants.BALL_KICK_LEFT_X_FAR ):
        return True
    return False
