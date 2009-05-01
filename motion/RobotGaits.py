import man.motion as motion

MARVIN_FAST_GAIT =  motion.GaitCommand(31.00, # com height
                                       1.40,  # hip offset x
                                       0.0,  # x-axis angle offset
                                       0.50,  # step duration
                                       0.15,   # fraction in double support
                                       1.55,  # stepHeight
                                       0.0,   # footLengthX
                                       0.4,   # zmp static percentage
                                       5.0,   # left swing hip roll addition
                                       5.0,   # right swing hip roll addition
                                       0.90,  # left zmp off
                                       0.90,  # right zmp off
                                       10.0,  # max x speed
                                       5.0,   # max y speed
                                       30.0,  # max theta speed()
                                       0.0)   #sensorScale

TRILLIAN_FAST_GAIT =  motion.GaitCommand(31.00, # com height
                                         1.40,  # hip offset x
                                         0.0,  # x-axis angle offset
                                         0.50,  # step duration
                                         0.15,   # fraction in double support
                                         1.55,  # stepHeight
                                         0.0,   # footLengthX
                                         0.4,   # zmp static percentage
                                         6.0,   # left swing hip roll addition
                                         6.0,   # right swing hip roll addition
                                         0.9,  # left zmp off
                                         0.9,  # right zmp off
                                         10.0,  # max x speed
                                         5.0,   # max y speed
                                         30.0,  # max theta speed()
                                         0.0)   #sensorScale
SLARTI_FAST_GAIT =  motion.GaitCommand(31.00, # com height
                                       1.40,  # hip offset x
                                       0.0,  # x-axis angle offset
                                       0.50,  # step duration
                                       0.15,   # fraction in double support
                                       1.55,  # stepHeight
                                       0.0,   # footLengthX
                                       0.4,   # zmp static percentage
                                       6.5,   # left swing hip roll addition
                                       6.5,   # right swing hip roll addition
                                       0.60,  # left zmp off
                                       0.60,  # right zmp off
                                       10.0,  # max x speed
                                       5.0,   # max y speed
                                       30.0,  # max theta speed()
                                       0.0)   #sensorScale

ZAPHOD_FAST_GAIT =  motion.GaitCommand(31.00, # com height
                                       1.40,  # hip offset x
                                       0.0,  # x-axis angle offset
                                       0.50,  # step duration
                                       0.15,   # fraction in double support
                                       1.55,  # stepHeight
                                       0.0,   # footLengthX
                                       0.4,   # zmp static percentage
                                       6.0,   # left swing hip roll addition
                                       5.0,   # right swing hip roll addition
                                       0.90,  # left zmp off
                                       0.90,  # right zmp off
                                       10.0,  # max x speed
                                       5.0,   # max y speed
                                       30.0,   # max theta speed()
                                       0.0)   #sensorScale


#MISC EXPERIMENTAL GAITS


TRILLIAN_SLOW_GAIT = motion.GaitCommand(31.00, # com height
                                        1.40,  # hip offset x
                                        0.0,  # x-axis angle offset
                                        2.00,  # step duration
                                        0.7,   # fraction in double support
                                        1.65,  # stepHeight
                                        0.0,   # footLengthX
                                        0.3,   # zmp static percentage
                                        5.0,   # left swing hip roll addition
                                        5.0,   # right swing hip roll addition
                                        0.85,  # left zmp off
                                        0.85,  # right zmp off
                                        10.0,  # max x speed
                                        5.0,   # max y speed
                                        30.0,   # max theta speed()
                                        1.0)   #sensorScale

TRILLIAN_MEDIUM_GAIT = motion.GaitCommand(31.00, # com height
                                          1.40,  # hip offset x
                                          0.0,  # x-axis angle offset
                                          1.00,  # step duration
                                          0.5,   # fraction in double support
                                          0.0,  # stepHeight
                                          0.0,   # footLengthX
                                          0.3,   # zmp static percentage
                                          10.0,   # left swing hip roll addition
                                          10.0,   # right swing hip roll addition
                                          0.6,  # left zmp off
                                          0.6,  # right zmp off
                                          7.0,  # max x speed
                                          4.0,   # max y speed
                                          30.0,   # max theta speed()
                                          1.0)   #sensorScale

ZAPHOD_MEDIUM_GAIT = motion.GaitCommand(31.00, # com height
                                        1.40,  # hip offset x
                                        0.0,  # x-axis angle offset
                                        1.00,  # step duration
                                        0.5,   # fraction in double support
                                        0.0,  # stepHeight
                                        0.0,   # footLengthX
                                        0.3,   # zmp static percentage
                                        10.0,   # left swing hip roll addition
                                        10.0,   # right swing hip roll addition
                                        0.6,  # left zmp off
                                        0.6,  # right zmp off
                                        7.0,  # max x speed
                                        4.0,   # max y speed
                                        30.0,   # max theta speed()
                                        1.0)   #sensorScale

SLARTI_MEDIUM_GAIT = motion.GaitCommand(31.00, # com height
                                        1.40,  # hip offset x
                                        0.0,  # x-axis angle offset
                                        1.00,  # step duration
                                        0.5,   # fraction in double support
                                        0.0,  # stepHeight
                                        0.0,   # footLengthX
                                        0.3,   # zmp static percentage
                                        10.0,   # left swing hip roll addition
                                        10.0,   # right swing hip roll addition
                                        0.6,  # left zmp off
                                        0.6,  # right zmp off
                                        7.0,  # max x speed
                                        4.0,   # max y speed
                                        30.0,   # max theta speed()
                                        1.0)   #sensorScale


MARVIN_MEDIUM_GAIT = motion.GaitCommand(31.00, # com height
                                        1.40,  # hip offset x
                                        0.0,  # x-axis angle offset
                                        1.00,  # step duration
                                        0.5,   # fraction in double support
                                        0.5,  # stepHeight
                                        0.0,   # footLengthX
                                        0.3,   # zmp static percentage
                                        10.0,   # left swing hip roll addition
                                        10.0,   # right swing hip roll addition
                                        0.6,  # left zmp off
                                        0.6,  # right zmp off
                                        7.0,  # max x speed
                                        4.0,   # max y speed
                                        30.0,   # max theta speed()
                                        1.0)   #sensorScale


MARVIN_MEDIUM_SENSORS_GAIT = motion.GaitCommand(31.00,# com height
                                                1.40, # hip offset x
                                                0.0,  # x-axis angle offset
                                                1.00, # step duration
                                                0.5,  # fraction in double support
                                                0.5,  # stepHeight
                                                0.0,  # footLengthX
                                                0.3,  # zmp static percentage
                                                10.0, # left swing hip roll addition
                                                10.0, # right swing hip roll addition
                                                0.6,  # left zmp off
                                                0.6,  # right zmp off
                                                7.0,  # max x speed
                                                4.0,  # max y speed
                                                30.0,  # max theta speed()
                                                1.0)   #sensorScale
#DEFAULT GAIT ASSIGNMENTS

TRILLIAN_GAIT = TRILLIAN_MEDIUM_GAIT
ZAPHOD_GAIT   =   ZAPHOD_MEDIUM_GAIT
SLARTI_GAIT   =   SLARTI_MEDIUM_GAIT
MARVIN_GAIT   =   MARVIN_MEDIUM_GAIT

TRILLIAN_TURN_GAIT = TRILLIAN_MEDIUM_GAIT
ZAPHOD_TURN_GAIT   =   ZAPHOD_MEDIUM_GAIT
SLARTI_TURN_GAIT   =   SLARTI_MEDIUM_GAIT
MARVIN_TURN_GAIT   =   MARVIN_MEDIUM_GAIT
