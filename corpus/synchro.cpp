
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

#include <map>
#include <string>
#include <pthread.h>
#include "synchro.h"

using namespace std;

Event::Event (string _name)
    : name(_name), signalled(false)
{
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}

Event::~Event ()
{
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

void Event::await ()
{
    pthread_mutex_lock(&mutex);

    if (!signalled)
        pthread_cond_wait(&cond, &mutex);
    signalled = false;

    pthread_mutex_unlock(&mutex);
}

bool Event::poll ()
{
    pthread_mutex_lock(&mutex);

    const bool result = signalled;
    signalled = false;

    pthread_mutex_unlock(&mutex);
    return result;
}

void Event::signal ()
{
    pthread_mutex_lock(&mutex);

    signalled = true;
    pthread_cond_signal(&cond);

    pthread_mutex_unlock(&mutex);
}

Synchro::Synchro ()
    : events()
{
}

Synchro::~Synchro ()
{
    for (map<string, Event*>::iterator itr = events.begin();
            itr != events.end(); itr++) {
        delete itr->second;
    }
}

Event* Synchro::create (string name)
{
    map<string, Event*>::iterator itr = events.find(name);
    if (itr != events.end())
        events[name] = new Event(name);
    return events[name];
}

const map<string, Event*>& Synchro::available ()
{
    return events;
}

void Synchro::await (Event* ev)
{
    ev->await();
}

bool Synchro::poll (Event* ev)
{
    return ev->poll();
}

void Synchro::signal (Event* ev)
{
    ev->signal();
}
