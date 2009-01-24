

#include <boost/shared_ptr.hpp>
using namespace boost;

#include "synchro.h"
#include "Motion.h"
#include "_motionmodule.h"
#include "SimulatorEnactor.h"
#include "SimulatorEnactor.h"

#ifdef NAOQI1
Motion::Motion (ALPtr<ALMotionProxy> _proxy,shared_ptr<Synchro> _synchro, Sensors *s)
#else
Motion::Motion (ALMotionProxy * _proxy,shared_ptr<Synchro> _synchro, Sensors *s)
#endif
    : Thread(_synchro, "MotionCore"),
      switchboard(s),
#ifdef NAOQI1
      enactor(new ALEnactor(&switchboard,_proxy)),
#else
      enactor(new SimulatorEnactor(&switchboard)),
#endif
      interface(&switchboard)
{
  set_motion_interface(&interface);
  c_init_motion();
}

Motion::~Motion() {
    delete enactor;
}

int Motion::start() {
    // Start the enactor thread
    enactor->start();
    switchboard.start();

    return Thread::start();
}

void Motion::stop() {
    switchboard.stop();
    enactor->stop();
    Thread::stop();
}

void Motion::run(){
    cout <<"Motion::run"<<endl;
    switchboard.run();
}
