
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

#ifndef _HeadProvider_h_DEFINED
#define _HeadProvider_h_DEFINED

#include <vector>
#include <queue>
#include <boost/shared_ptr.hpp>

#include "MotionProvider.h"
#include "ChainQueue.h"
#include "HeadJointCommand.h"
#include "Sensors.h"
#include "ChopShop.h"
#include "Kinematics.h"

class HeadProvider : public MotionProvider {
public:
    HeadProvider(float motionFrameLength, boost::shared_ptr<Sensors> s);
    virtual ~HeadProvider();

    void requestStopFirstInstance();
    void calculateNextJoints();

	void enqueueSequence(vector<HeadJointCommand*> &seq);
	void setCommand(const MotionCommand* command) {
		setCommand(reinterpret_cast<const HeadJointCommand*>(command));
	}
	void setCommand(const HeadJointCommand* command);

private:
    void setActive();
    bool isDone();

    boost::shared_ptr<Sensors> sensors;
    float FRAME_LENGTH_S;
    ChopShop chopper;
    std::vector< std::vector<float> > nextJoints;

	ChainQueue headQueue;
	// Queue of all future commands
	std::queue<const HeadJointCommand*> headCommandQueue;

    pthread_mutex_t head_mutex;

    std::vector<float> getCurrentHeads();
    void setNextHeadCommand();
};

#endif
