# gait minimum and maximum tuples - defines space we optimize over
# if gaitMins[i] = gaitMaxs[i] then the variable won't be optimized, and
# is only a placeholder
# parameters we're optimizing are marked with **

def gaitToArray(gait):
    list = []
    for section in gait:
        for value in section:
            list.append(value)
    return list

# the opposite of gaitToArray
# parses a list of the correct arity
def arrayToGaitTuple(array):
    assert len(array) == 44

    stance = tuple(array[0:6])
    step = tuple(array[6:18])
    zmp = tuple(array[18:24])
    hack = (array[25], array[25]) # joint hack is always the same L/R!
    sensor = tuple(array[26:34])
    stiffness = tuple(array[34:40])
    odo = tuple(array[40:43])
    arm = tuple(array[43:44])

    return (stance, step, zmp, hack, sensor, stiffness, odo, arm)

stanceMin = (31.00, # CoM height
            0.0,   #** Forward displacement of CoM
            10.0,  # Horizontal distance between feet
            0.0,   #** Body angle around y axis
            0.0,   # Angle between feet
            0.1)   # Time to transition to/from this stance

stanceMax = (31.00, # CoM height
             10.0,  #** Forward displacement of CoM
             10.0,  # Horizontal distance between feet
             10.0,  #** Body angle around y axis
             0.0,   # Angle between feet
             0.1)   # Time to transition to/from this stance

stepMin = (0.2,   #** step duration
           0.05,  #** fraction in double support
           0.1,   #** stepHeight
           -20.0, #** step lift
           15.0,  # max x speed
           -5.0,  # max x speed
           15.0,  # max y speed
           45.0,  # max theta speed()
           7.0,   # max x accel
           7.0,   # max y accel
           20.0,  # max theta speed()
           1.0)   # walk gait = true

stepMax = (1.0,   #** step duration
           0.9,   #** fraction in double support
           5.0,   #** stepHeight
           0.0,   #** step lift
           15.0,  # max x speed
           -5.0,  # max x speed
           15.0,  # max y speed
           45.0,  # max theta speed()
           7.0,   # max x accel
           7.0,   # max y accel
           20.0,  # max theta speed()
           1.0)   # walk gait = true

zmpMin = zmpMax = (0.0,   # footCenterLocX
                   0.3,   # zmp static percentage
                   0.45,  # left zmp off
                   0.45,  # right zmp off
                   0.01,  # strafe zmp offse
                   6.6)   # turn zmp offset

hackMin = (0.0,   #** joint hack
           0.0)   #** joint hack

hackMax =  (10.0,  #** joint hack
           10.0)  #** joint hack

sensorMin = (0.0,   # Feedback type (1.0 = spring, 0.0 = old)
             0.0,   #** angle X scale (gamma)
             0.0,   #** angle Y scale (gamma)
             250.0, # X spring constant k (kg/s^2)
             100.0, # Y spring constant k (kg/s^2)
             7.0,   # max angle X (compensation)
             7.0,   # max angle Y
             45.0)  # max angle vel (change in compensation)

sensorMax = (0.0,   # Feedback type (1.0 = spring, 0.0 = old)
             1.0,   #** angle X scale (gamma)
             1.0,   #** angle Y scale (gamma)
             250.0, # X spring constant k (kg/s^2)
             100.0, # Y spring constant k (kg/s^2)
             7.0,   # max angle X (compensation)
             7.0,   # max angle Y
             45.0)  # max angle vel (change in compensation)

stiffnessMin = stiffnessMax = (0.85,  # hipStiffness
                               0.3,   # kneeStiffness
                               0.4,   # anklePitchStiffness
                               0.3,   # ankleRollStiffness
                               0.1,   # armStiffness
                               0.5)   # arm pitch

odoMin = odoMax = (1.0,   # xOdoScale
                   1.0,   # yOdoScale
                   1.0)   # thetaOdoScale

armMin = armMax = (0.0,)  # arm config

gaitMins = (stanceMin,
            stepMin,
            zmpMin,
            hackMin,
            sensorMin,
            stiffnessMin,
            odoMin,
            armMin)

gaitMaxs = (stanceMax,
            stepMax,
            zmpMax,
            hackMax,
            sensorMax,
            stiffnessMax,
            odoMax,
            armMax)

