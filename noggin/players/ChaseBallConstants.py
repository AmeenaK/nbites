# Component Switches
SUPER_SAFE_KICKS = False # Only kick straight when we see the goal
USE_LOC_CHASE = False

# Transitions' Constants
# Ball on and off frame thresholds
BALL_ON_THRESH = 2
BALL_OFF_THRESH = 12
# Value to stop spinning to ball and approach
BALL_APPROACH_BEARING_THRESH = 10
# Value to start spinning to ball
BALL_APPROACH_BEARING_OFF_THRESH = 17
# Should position for kick
BALL_POS_KICK_DIST_THRESH = 15.0
BALL_POS_KICK_BEARING_THRESH = 30

# States' constants
# turnToBall
FIND_BALL_SPIN_SPEED = 25.0
BALL_SPIN_SPEED = 15.0
BALL_SPIN_GAIN = 0.4
MIN_BALL_SPIN_SPEED = 5

# approachBall() values
APPROACH_X_GAIN = 0.22
APPROACH_SPIN_SPEED = 5
MIN_APPROACH_SPIN_SPEED = 5
APPROACH_SPIN_GAIN = 0.4
MAX_X_SPEED = 7.5
MIN_X_SPEED = -7.5

# positionForKick() values
BALL_KICK_LEFT_Y_L = 8
BALL_KICK_LEFT_Y_R = 4
BALL_KICK_LEFT_X_CLOSE = 2
BALL_KICK_LEFT_X_FAR = 14

# Values for controlling the strafing
MAX_Y_SPEED = 3.0
MIN_Y_SPEED = -MAX_Y_SPEED
MIN_Y_MAGNITUDE = 1.5

# Keep track of what gait we're using
FAST_GAIT = "fastGait"
NORMAL_GAIT = "normalGait"

# Obstacle avoidance stuff
AVOID_OBSTACLE_DIST = 40.0 #cm
