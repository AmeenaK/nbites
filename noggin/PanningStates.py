import man.motion.HeadMoves as HeadMoves

TRACKER_FRAMES_ON_TRACK_THRESH = 1 #num frms after which to switch to scanfindbl

def scanBall(tracker):
    ball = tracker.brain.ball

    if tracker.target == ball and \
            tracker.target.framesOn >= TRACKER_FRAMES_ON_TRACK_THRESH:
        return tracker.goNow('ballTracking')

    if not tracker.brain.motion.isHeadActive():
        lastBallDist = ball.lastVisionDist
        if lastBallDist > HeadMoves.HIGH_SCAN_CLOSE_BOUND:
            tracker.execute(HeadMoves.HIGH_SCAN_BALL)

        elif lastBallDist > HeadMoves.MID_SCAN_CLOSE_BOUND and \
                lastBallDist < HeadMoves.MID_SCAN_FAR_BOUND:
            tracker.execute(HeadMoves.MID_UP_SCAN_BALL)
        else:
            tracker.execute(HeadMoves.LOW_SCAN_BALL)

    return tracker.stay()

def scanning(tracker):
    if (tracker.firstFrame() or not
        tracker.brain.motion.isHeadActive()):
        tracker.execute(tracker.currentHeadScan)
    return tracker.stay()

def locPans(tracker):
    if tracker.firstFrame() \
            or not tracker.brain.motion.isHeadActive():
        tracker.execute(HeadMoves.QUICK_PANS)
    return tracker.stay()

def panLeftOnce(tracker):
    if tracker.firstFrame():
        tracker.execute(HeadMoves.PAN_LEFT)

    if not tracker.brain.motion.isHeadActive():
        return tracker.goLater(tracker.lastDiffState)

    return tracker.stay()

def panRightOnce(tracker):
    if tracker.firstFrame():
        tracker.execute(HeadMoves.PAN_RIGHT)

    if not tracker.brain.motion.isHeadActive():
        return tracker.goLater(tracker.lastDiffState)

    return tracker.stay()

def postScan(tracker):
    if tracker.firstFrame() \
            or not tracker.brain.motion.isHeadActive():
        tracker.execute(HeadMoves.POST_SCAN)
    return tracker.stay()

def activeLocScan(tracker):
    if tracker.firstFrame() \
            or not tracker.brain.motion.isHeadActive():
        tracker.execute(HeadMoves.QUICK_PANS)
    if tracker.target.on:
        return tracker.goLater('activeTracking')
    return tracker.stay()
