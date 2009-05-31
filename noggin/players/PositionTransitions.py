import PositionConstants as constants

####### POSITIONING STUFF ##############

def shouldTurnToBall_fromAtBallPosition(player):
    """
    Should we spin towards the ball, if already at our position
    """
    ball = player.brain.ball
    return (ball.on and
        abs(ball.bearing > (constants.BALL_SPIN_POSITION_THRESH +
                            constants.BALL_SPIN_BUFFER)))

def atSpinBallDir(player):
    """
    Should we stop spinning because we are facing the ball
    """
    ball = player.brain.ball
    return (ball.on and
            abs(ball.bearing < constants.BALL_SPIN_POSITION_THRESH))

def shouldSpinFindBallPosition(player):
    """
    Should we spin to find the ball
    """
    if player.stateTime >= constants.BALL_OFF_SPIN_TIME:
        return True
    return False

def shouldKickAtPosition(player):
    """
    Is the ball close enough that we should kick
    """
    return (player.brain.ball.on and
            player.brain.ball.dist < constants.AT_POSITION_KICK_DIST)
