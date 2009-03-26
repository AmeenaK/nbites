
// This file is part of Man, a robotic perception, locomotion, and
// team strategy application created by the Northern Bites RoboCup
// team of Bowdoin College in Brunswick, Maine, for the Aldebaran
// Nao robot.
//
// Man is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Man is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser Public License for more details.
//
// You should have received a copy of the GNU General Public License
// and the GNU Lesser Public License along with Man.  If not, see
// <http://www.gnu.org/licenses/>.

#ifndef _ScriptedProvider_h_DEFINED
#define _ScriptedProvider_h_DEFINED

#include <vector>
#include <queue>
#include <boost/shared_ptr.hpp>

#include "MotionProvider.h"
#include "ChainQueue.h"
#include "BodyJointCommand.h"
#include "Sensors.h"
#include "ChopShop.h"
#include "Kinematics.h"

using std::vector;
using std::queue;
using boost::shared_ptr;

class ScriptedProvider : public MotionProvider {
public:
    ScriptedProvider(float motionFrameLength, shared_ptr<Sensors> s);
    virtual ~ScriptedProvider();

    void requestStopFirstInstance();
    void calculateNextJoints();

	void enqueueSequence(std::vector<const BodyJointCommand*> &seq);
	void setCommand(const BodyJointCommand * command);

private:
	shared_ptr<Sensors> sensors;
	float FRAME_LENGTH_S;
	ChopShop chopper;
	vector<vector<float> > nextJoints;

	// The current chopped command which is being enacted
	shared_ptr<ChoppedCommand> currCommand;

	// Queue to hold the next body commands
	queue<const BodyJointCommand*> bodyCommandQueue;

	pthread_mutex_t scripted_mutex;

	shared_ptr<vector <vector <float> > > getCurrentChains();
	void setNextBodyCommand();
    void setActive();
	bool isDone();
	bool currCommandEmpty();
    bool commandQueueEmpty();

};

#endif
