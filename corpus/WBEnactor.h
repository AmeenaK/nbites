#ifndef WBEnactor_h
#define WBEnactor_h

#include "Sensors.h"
#include "Transcriber.h"
#include "ThreadedMotionEnactor.h"

class WBEnactor : public ThreadedMotionEnactor {
public:
    WBEnactor(boost::shared_ptr<Sensors> _sensors,
              boost::shared_ptr<Synchro> synchro,
              boost::shared_ptr<Transcriber> transcriber);
    virtual ~WBEnactor();

    void postSensors();
    void sendCommands();

    void run();

protected:
    boost::shared_ptr<Sensors> sensors;
    boost::shared_ptr<Transcriber> transcriber;

    static const int MOTION_FRAME_RATE;
    static const float MOTION_FRAME_LENGTH_uS; // in microseconds
    static const float MOTION_FRAME_LENGTH_S; // in seconds

};

#endif
