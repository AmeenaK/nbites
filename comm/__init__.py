
try:
    from ._comm import (Comm,
                        GameController,
                        PENALTY_BALL_HOLDING,
                        PENALTY_DAMAGE,
                        PENALTY_GOALIE_PUSHING,
                        PENALTY_ILLEGAL_DEFENDER,
                        PENALTY_ILLEGAL_DEFENSE,
                        PENALTY_LEAVING,
                        PENALTY_MANUAL,
                        PENALTY_NONE,
                        PENALTY_OBSTRUCTION)
except ImportError:
    import sys
    print >>sys.stderr, "**** WARNING - No backend _comm module located ****"
    inst = None
    
