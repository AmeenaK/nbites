#ifndef _ALEnactor_h_DEFINED
#define _ALEnactor_h_DEFINED

#ifdef NAOQI1

#include "almotionproxy.h"
#include "alptr.h"
using namespace AL;

#include "Sensors.h"
#include "MotionEnactor.h"


class ALEnactor : public MotionEnactor {
public:
    ALEnactor(MotionSwitchboard * _switchboard, ALPtr<ALMotionProxy> _mproxy,
        Sensors * s)
        : MotionEnactor(_switchboard), mproxy(_mproxy),sensors(s) {};
    virtual ~ALEnactor() { };

    virtual void run();

    void postSensors(); //This should prob. be moved into MotionEnactor
private:
    ALPtr<ALMotionProxy>  mproxy;
    Sensors *sensors;
    static const int MOTION_FRAME_RATE = 50;
    static const float MOTION_FRAME_LENGTH_uS = // in microseconds
      // 1 second * 1000 ms/s * 1000 us/ms
      1.0f * 1000.0f * 1000.0f / MOTION_FRAME_RATE;
    static const float MOTION_FRAME_LENGTH_S = // in seconds
      1.0f / MOTION_FRAME_RATE;
};

#endif //NAOQI1

#endif//_ALEnactor_h_DEFINED
