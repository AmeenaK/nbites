
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef Synchro_h_DEFINED
#define Synchro_h_DEFINED

#include <map>
#include <string>
#include <pthread.h>
#include <boost/shared_ptr.hpp>


struct MutexDeleter
{
    void operator() (pthread_mutex_t *mutex) { pthread_mutex_destroy(mutex); }
};

class Event
{
  friend class Synchro;

  private:
    Event(std::string name);
    Event(std::string name, boost::shared_ptr<pthread_mutex_t> mutex);
  public:
    virtual ~Event();

  public:
    void await();
    bool poll();
    void signal();

  public:
    const std::string name;

  private:
    boost::shared_ptr<pthread_mutex_t> mutex;
    pthread_cond_t cond;
    bool signalled;
};


class Synchro
{
  public:
    Synchro();
    virtual ~Synchro();

  public:
    // Obtain a map of event names to Events
    const std::map<std::string, boost::shared_ptr<Event> >& available();
    // Create a new event with the given name
    boost::shared_ptr<Event> create(std::string event_name);
    boost::shared_ptr<Event> create(std::string event_name,
                                    boost::shared_ptr<pthread_mutex_t> mutex);

    // Wait for an event to be signalled, and clear the signal
    void await(Event* ev);
    // Check if an event has been signalled, and clear the signal
    bool poll(Event* ev);
    // Signal an event has occurred
    void signal(Event* ev);

  private:
    std::map<std::string, boost::shared_ptr<Event> > events;
};

class TriggeredEvent
{
  public:
    TriggeredEvent() { }
    virtual ~TriggeredEvent() { }

    virtual bool poll() = 0;
    virtual void await_on() = 0;
    virtual void await_off() = 0;
    virtual void await_flip() = 0;
};

static const std::string TRIGGER_ON_SUFFIX("_on");
static const std::string TRIGGER_OFF_SUFFIX("_off");
static const std::string TRIGGER_FLIP_SUFFIX("_flip");

class Trigger
  : public TriggeredEvent
{
  public:
    Trigger(boost::shared_ptr<Synchro> _synchro, std::string name,
            bool _value=false);
    ~Trigger() { }

    void flip();
    void on();
    void off();

    bool poll();
    void await_on();
    void await_off();
    void await_flip();

  private:
    inline int lock() { return pthread_mutex_lock(mutex.get()); }
    inline int release() { return pthread_mutex_unlock(mutex.get()); }

  private:
    boost::shared_ptr<pthread_mutex_t> mutex;
    boost::shared_ptr<Event> on_event;
    boost::shared_ptr<Event> off_event;
    boost::shared_ptr<Event> flip_event;
    bool value;
};

class Thread
{
  public:
    Thread(boost::shared_ptr<Synchro> _synchro, std::string _name);
    virtual ~Thread();

  public:
    // To start and stop the thread.  Thread may be run repeatedly, but make
    // sure the thread has stopped before starting it once more.
    int start();
    void stop();

    // Overload this method to run your thread code
    virtual void run();

    // These are/should only fired once!  be careful, or deadlock could ensue
    const boost::shared_ptr<TriggeredEvent> getTrigger() { return trigger; }

  private:
    static void* runThread(void* _thread);

  public:
    const std::string name;

  private:
    pthread_t thread;

  protected:
    boost::shared_ptr<Synchro> synchro;
    bool running;
    boost::shared_ptr<Trigger> trigger;
};

#endif // Synchro_h_DEFINED
