#ifndef _ALEnactor_h_DEFINED
#define _ALEnactor_h_DEFINED

#ifdef NAOQI1

#include <boost/shared_ptr.hpp>
#include "alerror.h"
#include "almotionproxy.h"
#include "almemoryproxy.h"
#include "alptr.h"
#include "albroker.h"
#include "alproxy.h"
#include "dcmproxy.h"
#include "almemoryfastaccess.h"

#include "motionconfig.h"
#include "Sensors.h"
#include "MotionEnactor.h"


class ALEnactor : public MotionEnactor {
public:
    ALEnactor(AL::ALPtr<AL::ALBroker> _pbroker, boost::shared_ptr<Sensors> s)
        : MotionEnactor(), broker(_pbroker), sensors(s){
        try{
            alfastaccess =
                AL::ALPtr<ALMemoryFastAccess >(new ALMemoryFastAccess());
        } catch(AL::ALError &e){
            cout << "Failed to initialize proxy to ALFastAccess"<<endl;
        }
        try {
            almemory = broker->getMemoryProxy();
        } catch(AL::ALError &e){
            cout << "Failed to initialize proxy to ALMemory" << endl;
        }
        try {
            almotion = broker->getMotionProxy();
        } catch(AL::ALError &e){
            cout << "Failed to initialize proxy to ALMotion" << endl;
        }
        try {
            dcm = AL::ALPtr<AL::DCMProxy>(new AL::DCMProxy(broker));
        } catch(AL::ALError &e) {
            cout << "Failed to initialize proxy to DCM" << endl;
        }
        //starting out we want to set our motion angles to the sensed
        //position
        motionCommandAngles = almotion->getBodyAngles();
        for (unsigned int i=0; i<motionCommandAngles.size();i++)
            cout << "initial motion angle at  " << i << " is "
                 << motionCommandAngles.at(i) << endl;

        initSyncWithALMemory();
    };
    virtual ~ALEnactor() { };

    virtual void run();

    virtual void postSensors();

private:
    void syncWithALMemory();
    void initSyncWithALMemory();
private:
    AL::ALPtr<AL::ALBroker> broker;
    AL::ALPtr<AL::ALMotionProxy>  almotion;
    AL::ALPtr<AL::ALMemoryProxy>  almemory;
    AL::ALPtr<ALMemoryFastAccess> alfastaccess;
    AL::ALPtr<AL::DCMProxy> dcm;
    boost::shared_ptr<Sensors> sensors;
    std::vector<float> motionCommandAngles;
    static const int MOTION_FRAME_RATE;
    static const float MOTION_FRAME_LENGTH_uS; // in microseconds
    static const float MOTION_FRAME_LENGTH_S; // in seconds
};

#endif //NAOQI1

#endif//_ALEnactor_h_DEFINED
