
#ifndef _GameController_h_DEFINED
#define _GameController_h_DEFINED

#include <vector>
#include <pthread.h>
#include "GameControllerConfig.h"
#include "RoboCupGameControlData.h"
#include "Sensors.h"

class GameController;

#ifdef USE_PYTHON_GC
#include <Python.h>

typedef struct PyGameController_t {
  PyObject_HEAD
  GameController *_gc;
} PyGameController;

extern PyObject* PyGameController_new(GameController *_gc);

extern PyTypeObject PyGameControllerType;

#endif // USE_PYTHON_GC

class GameController
{
#ifdef USE_PYTHON_GC
    friend class PyGameController_t;
#endif
public:
    GameController();
    virtual ~GameController() {} 
    
    /* interface functions for RoboCupGameControlData
       use these for on demand information rather than waiting
       for the updates */
    const TeamInfo* getMyTeam() { return myTeam; }
    const RoboCupGameControlData& getGameData() { return controlData; }

    void handle_packet(const char *msg, int len);


    // Public data access interface
    const uint8 team();
    const uint8 color();
    const uint8 player();
    const uint8 kickOffTeam();
    const GCGameState gameState();
    const GCPenalty penalty();
    const GCPenalty penalties(uint16 player);
    const uint16 penaltySeconds();
    const uint16 penaltySeconds(uint16 player);

    void setTeam(uint8 team);
    void setColor(uint8 color);
    void setPlayer(uint8 player);
    void setKickOffTeam(uint8 team);
    void setGameState(GCGameState state);
    void setPenalty(GCPenalty penalized);

    
  private:
    // check the validity of the given message and store the data
    // (if valid) into the referenced RoboCupGameControlData struct
    bool validatePacket(const char *msg, int len, RoboCupGameControlData &packet);    
    void swapTeams(int team);
    void rawSwapTeams(RoboCupGameControlData& data);
   
  private:
    /* local copy of the GameController data */
    RoboCupGameControlData controlData;
    TeamInfo* myTeam;
    uint8 playerNumber;

    // thread mutex to enable locking and thread-safe data access
    pthread_mutex_t mutex;
};

#endif
