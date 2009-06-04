

#include <iostream>
#include <string.h>
#include <math.h>

#include "commconfig.h"

#include "GameController.h"
#include "RoboCupGameControlData.h"

using namespace std;

const uint8 GameController::NUM_TEAMS = 2;

bool DEBUG = false;

// print error messages when bad packets are received
#undef  DEBUG_INVALID_PACKETS
//#define DEBUG_INVALID_PACKETS

static void
printGCData(RoboCupGameControlData& packet)
{
  cout << "Packet received::" << endl;
  printf( "  header         == %*s\n", 4, packet.header);
  cout << "  version        == " << (int)packet.version << endl;
  cout << "  playersPerTeam == " << (int)packet.playersPerTeam << endl;
  cout << "  state          == " << (int)packet.state << endl;
  cout << "  firstHalf      == " << (int)packet.firstHalf << endl;
  cout << "  kickOffTeam    == " << (int)packet.kickOffTeam << endl;
  cout << "  secondaryState == " << (int)packet.secondaryState << endl;
  cout << "  dropInTeam     == " << (int)packet.dropInTeam << endl;
  cout << "  dropInTime     == " << packet.dropInTime << endl;
  cout << "  secsRemaining  == " << packet.secsRemaining << endl;

  cout << "  teams[0]" << endl;
  cout << "    teamNumber   == " << (int)packet.teams[0].teamNumber << endl;
  cout << "    teamColor    == " << (int)packet.teams[0].teamColor << endl;
  cout << "  teams[1]" << endl;
  cout << "    teamNumber   == " << (int)packet.teams[1].teamNumber << endl;
  cout << "    teamColor    == " << (int)packet.teams[1].teamColor << endl;
}


/* class constructor */ 
GameController::GameController()
  : controlData(), myTeam(&controlData.teams[TEAM_BLUE]), playerNumber(1)
{
  pthread_mutex_init (&mutex, NULL);
}

void
GameController::handle_packet(const char *msg, int len)
{
  RoboCupGameControlData packet;
  if (!validatePacket(msg, len, packet))
    return;

  if (packet.teams[TEAM_BLUE].teamColor != TEAM_BLUE)
    rawSwapTeams(packet);

  //beepOnUnfairPenalize(packet);
  
  // if data has changed
  if (memcmp(&packet, &controlData, sizeof(controlData)) != 0) {

    pthread_mutex_lock(&mutex);

    // update team info
    if (packet.teams[TEAM_BLUE].teamNumber == myTeam->teamNumber)
      myTeam = &controlData.teams[TEAM_BLUE];
    else if (packet.teams[TEAM_RED].teamNumber == myTeam->teamNumber)
      myTeam = &controlData.teams[TEAM_RED];

    // update global data
    controlData = packet;

    pthread_mutex_unlock(&mutex);
  }

}

bool
GameController::validatePacket(const char *msg, int len,
                               RoboCupGameControlData &packet)
{
  // ensure correct packet size
  if (len != sizeof(packet)) {
#ifdef DEBUG_INVALID_PACKETS
    cerr << "GameController::validatePacket() -- invalid packet length";
    cerr << endl;
    cerr << "  packet length == " << len << endl;
#endif
    return false;
  }

  // cast msg into packet data
  packet = *reinterpret_cast<const RoboCupGameControlData*>(msg);

  // check header
  if (memcmp(packet.header, GAMECONTROLLER_STRUCT_HEADER,
             sizeof(GAMECONTROLLER_STRUCT_HEADER)) != 0) {
#ifdef DEBUG_INVALID_PACKETS
    cerr << "GameController::validatePacket() -- invalid packet header";
    cerr << endl;
    cerr << "  header == " << packet.header << endl;
#endif
    return false;
  }

  // check version number
  if (packet.version != GAMECONTROLLER_STRUCT_VERSION) {
#ifdef DEBUG_INVALID_PACKETS
    cerr << "GameController::validatePacket() -- invalid version number";
    cerr << endl;
    cerr << "  version == " << packet.version << endl;
#endif
    return false;
  }

  pthread_mutex_lock(&mutex);

  // check team numbers
  if (packet.teams[TEAM_BLUE].teamNumber != myTeam->teamNumber &&
      packet.teams[TEAM_RED].teamNumber  != myTeam->teamNumber) {
#ifdef DEBUG_INVALID_PACKETS
    cerr << "GameController::validatePacket() -- invalid team number" << endl;
    cerr << "  team blue == " << (int)packet.teams[TEAM_BLUE].teamNumber;
    cerr << endl;
    cerr << "  team red  == " << (int)packet.teams[TEAM_RED].teamNumber;
    cerr << endl;
    cerr << "  our team  == " << (int)myTeam->teamNumber << endl;
#endif
    pthread_mutex_unlock(&mutex);
    return false;

  }else {
    pthread_mutex_unlock(&mutex);
    // passed all check, packet is valid
    return true;
  }
}

/* swap the teams in memory to maintain BLUE team first order */
void
GameController::rawSwapTeams (RoboCupGameControlData& data)
{
  TeamInfo tempTeam = data.teams[TEAM_BLUE];
  data.teams[TEAM_BLUE] = data.teams[TEAM_RED];
  data.teams[TEAM_RED] = tempTeam;
}

const uint32
GameController::timeRemaining()
{
  pthread_mutex_lock(&mutex);
  const uint32 time = controlData.secsRemaining;
  pthread_mutex_unlock(&mutex);
  return time;
}
const uint8
GameController::team()
{
  pthread_mutex_lock(&mutex);
  const uint8 team = myTeam->teamNumber;
  pthread_mutex_unlock(&mutex);
  return team;
}

const TeamInfo
GameController::teams(uint16 team)
{
  pthread_mutex_lock(&mutex);
  const TeamInfo info = controlData.teams[team];
  pthread_mutex_unlock(&mutex);
  return info;
}

const uint8
GameController::color()
{
  pthread_mutex_lock(&mutex);
  const uint8 color = myTeam->teamColor;
  pthread_mutex_unlock(&mutex);
  return color;
}

const uint8
GameController::player()
{
  pthread_mutex_lock(&mutex);
  const uint8 player = playerNumber;
  pthread_mutex_unlock(&mutex);
  return player;
}

const uint8
GameController::kickOffTeam()
{
  pthread_mutex_lock(&mutex);
  const uint8 kickOff = controlData.kickOffTeam;
  pthread_mutex_unlock(&mutex);
  return kickOff;
}

const GCGameState
GameController::gameState()
{
  pthread_mutex_lock(&mutex);
  const GCGameState state = static_cast<GCGameState>(controlData.state);
  pthread_mutex_unlock(&mutex);
  return state;
}

const GCPenalty
GameController::penalty()
{
  pthread_mutex_lock(&mutex);
  const GCPenalty penalized =
    static_cast<GCPenalty>(myTeam->players[playerNumber-1].penalty);
  pthread_mutex_unlock(&mutex);
  return penalized;
}

const GCPenalty
GameController::penalties(uint16 player)
{
  pthread_mutex_lock(&mutex);
  const GCPenalty penalized =
    static_cast<GCPenalty>(myTeam->players[player-1].penalty);
  pthread_mutex_unlock(&mutex);
  return penalized;
}

const uint16
GameController::penaltySeconds()
{
  pthread_mutex_lock(&mutex);
  const uint16 seconds = myTeam->players[playerNumber-1].secsTillUnpenalised;
  pthread_mutex_unlock(&mutex);
  return seconds;
}

const uint16
GameController::penaltySeconds(uint16 player)
{
  pthread_mutex_lock(&mutex);
  const uint16 seconds = myTeam->players[player-1].secsTillUnpenalised;
  pthread_mutex_unlock(&mutex);
  return seconds;
}

void
GameController::setTeam(uint8 team)
{
  pthread_mutex_lock(&mutex);
  myTeam->teamNumber = team;
  pthread_mutex_unlock(&mutex);
}

void
GameController::setColor(uint8 color)
{
  pthread_mutex_lock(&mutex);
  myTeam->teamColor = color;
  pthread_mutex_unlock(&mutex);
}

void
GameController::setPlayer(uint8 player)
{
  pthread_mutex_lock(&mutex);
  playerNumber = player;
  pthread_mutex_unlock(&mutex);
}

void
GameController::setKickOffTeam(uint8 kickOff)
{
  pthread_mutex_lock(&mutex);
  controlData.kickOffTeam = kickOff;
  pthread_mutex_unlock(&mutex);
}

void
GameController::setGameState(GCGameState state)
{
  pthread_mutex_lock(&mutex);
  controlData.state = static_cast<uint8>(state);
  pthread_mutex_unlock(&mutex);
}

void
GameController::setPenalty(GCPenalty penalty)
{
  pthread_mutex_lock(&mutex);
  myTeam->players[playerNumber-1].penalty = static_cast<uint8>(penalty);
  pthread_mutex_unlock(&mutex);
}



void GameController::advanceButtonClickState(){
    const GCGameState currentState = gameState();
    const GCPenalty currentPenalty  = penalty();

    //From the Nao Rules, when the chest button is pushed
    //We need to advance the states
    switch(currentState){
    case STATE_INITIAL:
        if(currentPenalty != PENALTY_NONE){
            setPenalty(PENALTY_NONE);
        }
        setGameState(STATE_PLAYING);
        break;
    case STATE_PLAYING:
        if(currentPenalty == PENALTY_NONE){
            setPenalty(PENALTY_MANUAL);
        }else{
            setPenalty(PENALTY_NONE);
        }
        break;
    default:
        break;
        //do nothing
    }
}
void GameController::toggleTeamColor(){
    //switch to the next team
    setColor((color()+1) % NUM_TEAMS);

}

void GameController::toggleKickoff(){

    const uint8 team1 = (controlData.teams[0]).teamNumber;
    const uint8 team2 = (controlData.teams[1]).teamNumber;

    if(kickOffTeam() == team1){
        setKickOffTeam(team2);
    }else if(kickOffTeam() == team2){
        setKickOffTeam(team1);
    }else{
//         cout << "GameController:: kickOffTeam is not consistent with TeamInfo"
//              << " Setting kickoff team to " << team()
//              << endl;
        setKickOffTeam(team());
    }
}
//
// Python GameController interface
// 

#ifdef USE_PYTHON_GC

// Python object allocation method.  Not used
extern PyObject* PyGameController_new(PyTypeObject* type, PyObject* args,
    PyObject *kwds);

// Python deallocation
extern void PyGameController_dealloc(PyObject* self);

// Generic getter method, used to retrieve C++ values into Python
extern PyObject* PyGameController_get(PyGameController* self, void* closure);
// Generic setter method, used to set C++ values from Python
extern int PyGameController_set(PyGameController* self, PyObject* val,
    void* closure);
extern PyObject* PyGameController_timeRemaining(PyGameController* self);
extern PyObject* PyGameController_players(PyGameController* self,
    PyObject* args);
extern PyObject* PyGameController_teams(PyGameController* self,
    PyObject* args);

enum PyGameController_attr {
  GC_TEAM = 0,
  GC_COLOR,
  GC_PLAYER,
  GC_KICKOFF,
  GC_STATE,
  GC_PENAL
};

static PyGetSetDef PyGameController_getsetters[] = {
  {"team", reinterpret_cast<getter>(PyGameController_get),
           reinterpret_cast<setter>(PyGameController_set),
    "This robot's team number",
    reinterpret_cast<void*>(GC_TEAM)},

  {"color", reinterpret_cast<getter>(PyGameController_get),
            reinterpret_cast<setter>(PyGameController_set),
    "This robot's team color",
    reinterpret_cast<void*>(GC_COLOR)},

  {"player", reinterpret_cast<getter>(PyGameController_get),
             reinterpret_cast<setter>(PyGameController_set),
    "This robot's player number",
    reinterpret_cast<void*>(GC_PLAYER)},

  {"kickOff", reinterpret_cast<getter>(PyGameController_get),
              reinterpret_cast<setter>(PyGameController_set),
    "The current kick-off team (0 for blue, 1 for red)",
    reinterpret_cast<void*>(GC_KICKOFF)},

  {"state", reinterpret_cast<getter>(PyGameController_get),
            reinterpret_cast<setter>(PyGameController_set),
    "The current GameController game state",
    reinterpret_cast<void*>(GC_STATE)},

  {"penalty", reinterpret_cast<getter>(PyGameController_get),
              reinterpret_cast<setter>(PyGameController_set),
    "This robot's penalized state",
    reinterpret_cast<void*>(GC_PENAL)},

  // Sentinel
  {NULL}
};

static PyMethodDef PyGameController_methods[] = {
  {"players", reinterpret_cast<PyCFunction>(PyGameController_players),
    METH_O,
    "Retrieve a tuple containing the penalty state and remaining seconds for "
    "the given robot"},

  {"teams", reinterpret_cast<PyCFunction>(PyGameController_teams),
    METH_O,
    "Retrieve a tuple containing the team color and team score for the given"
    "team."},

  {"timeRemaining", reinterpret_cast<PyCFunction>(PyGameController_timeRemaining),
    METH_NOARGS,
    "retrieve the time remaining in the half"
  },

  // Sentinel
  {NULL}
};

PyTypeObject PyGameControllerType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "comm.GameController",     /*tp_name*/
    sizeof(PyGameController),  /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)PyGameController_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "GameController object",   /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    PyGameController_methods,  /* tp_methods */
    0,                         /* tp_members */
    PyGameController_getsetters, /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    (newfunc)PyGameController_new, /* tp_new */
};


PyObject*
PyGameController_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyErr_SetString(PyExc_SystemError,
      "Cannot create PyGameController from Python");
  return NULL;
}

PyObject*
PyGameController_new (boost::shared_ptr<GameController> _gc)
{
  PyObject *self;

  self = PyGameControllerType.tp_alloc(&PyGameControllerType, 0);
  //self = PyRobotAccess Type.tp_alloc(&PyRobotAccessType,    0);
  if (self != NULL)
    reinterpret_cast<PyGameController*>(self)->_gc = _gc;

  return self;
}

void
PyGameController_dealloc (PyObject* self)
{
  self->ob_type->tp_free(self);
}

/** 
 * Retrieve a C++ GameController data attribute from Python.  Closure supplied
 * in definition determines the type requested.  Uses Py_BEGIN_ALLOW_THREADS
 * and Py_END_ALLOW_THREADS to prevent Python blocking in other threads while
 * the PyGameController class locks the GameController and accesses the data
 * attributes.
 */
PyObject*
PyGameController_get (PyGameController* self, void* closure)
{
  PyObject *result = NULL;

  Py_BEGIN_ALLOW_THREADS;
  switch (reinterpret_cast<int>(closure)) {
    case GC_TEAM:
      result = PyInt_FromLong(self->_gc->team());
      break;
    case GC_COLOR:
      result = PyInt_FromLong(self->_gc->color());
      break;
    case GC_PLAYER:
      result = PyInt_FromLong(self->_gc->player());
      break;
    case GC_KICKOFF:
      result = PyInt_FromLong(self->_gc->kickOffTeam());
      break;
    case GC_STATE:
      result = PyInt_FromLong(self->_gc->gameState());
      break;
    case GC_PENAL:
      result = PyInt_FromLong(self->_gc->penalty());
      break;
  }
  Py_END_ALLOW_THREADS;

  return result;
}

/**
 * Set a C++ GameController attribute from Python.  See PyGameController_get()
 * for threading information.
 */
int
PyGameController_set (PyGameController* self, PyObject* value, void* closure)
{
  if (!PyNumber_Check(value)) {
    PyErr_Format(PyExc_TypeError, "expected an int (%s given)",
        value->ob_type->tp_name);
    return -1;
  }

  int tmp, result = 0;

  Py_BEGIN_ALLOW_THREADS;
  switch (reinterpret_cast<int>(closure)) {
    case GC_TEAM:
      self->_gc->setTeam(static_cast<uint8>(PyInt_AsLong(value)));
      break;
    case GC_COLOR:
      tmp = PyInt_AsLong(value);
      if (tmp != TEAM_BLUE && tmp != TEAM_RED) {
        PyErr_Format(PyExc_ValueError,
            "expected a team color of %i or %i (%i given)", TEAM_BLUE,
            TEAM_RED, tmp);
        result = -1;
      }else
        self->_gc->setColor(static_cast<uint8>(tmp));
      break;
    case GC_PLAYER:
      self->_gc->setPlayer(static_cast<uint8>(PyInt_AsLong(value)));
      break;
    case GC_KICKOFF:
      tmp = PyInt_AsLong(value);
      if (tmp != TEAM_BLUE && tmp != TEAM_RED) {
        PyErr_Format(PyExc_ValueError,
            "expected a kick-off team (color) of %i or %i (%i given)",
            TEAM_BLUE, TEAM_RED, tmp);
        result = -1;
      }else
        self->_gc->setKickOffTeam(static_cast<uint8>(tmp));
      break;
    case GC_STATE:
      tmp = PyInt_AsLong(value);
      if (tmp < STATE_INITIAL || tmp > STATE_FINISHED) {
        PyErr_Format(PyExc_ValueError,
            "expected a game state from %i to %i (%i given)",
            STATE_INITIAL, STATE_FINISHED, tmp);
        result = -1;
      }else
        self->_gc->setGameState(static_cast<GCGameState>(tmp));
      break;
    case GC_PENAL:
      tmp = PyInt_AsLong(value);
      if (tmp < PENALTY_NONE || tmp > PENALTY_MANUAL) {
        PyErr_Format(PyExc_ValueError,
            "expected a penalty from %i to %i (%i given)",
             PENALTY_NONE, PENALTY_MANUAL, tmp);
        result = -1;
      }else
        self->_gc->setPenalty(static_cast<GCPenalty>(tmp));
      break;
  }
  Py_END_ALLOW_THREADS;

  return result;
}

PyObject*
PyGameController_timeRemaining (PyGameController* self)
{
  PyObject* time;
  Py_BEGIN_ALLOW_THREADS;
  time = PyInt_FromLong(self->_gc->timeRemaining());
  Py_END_ALLOW_THREADS;

  return time;
}

PyObject*
PyGameController_players (PyGameController* self, PyObject* args)
{
  if (!PyInt_Check(args)) {
    PyErr_Format(PyExc_TypeError,
        "expected an integer argument (%s given)", args->ob_type->tp_name);
    return NULL;
  }

  int i = PyInt_AsLong(args);
  if (i < 1 || i > DEF_NUM_PLAYERS) {
    PyErr_Format(PyExc_ValueError,
        "expected in integer from %i to %i (%i given)",
        1, DEF_NUM_PLAYERS, i);
    return NULL;
  }

  GCPenalty p;
  uint16 s;

  Py_BEGIN_ALLOW_THREADS;

  p = self->_gc->penalties(i);
  s = self->_gc->penaltySeconds(i);

  Py_END_ALLOW_THREADS;

  PyObject *t = PyTuple_New(2);
  if (t != NULL) {
    PyTuple_SET_ITEM(t, 0, PyInt_FromLong(p));
    PyTuple_SET_ITEM(t, 1, PyInt_FromLong(s));
  }

  return t;
}

PyObject*
PyGameController_teams (PyGameController* self, PyObject* args)
{
  if (!PyInt_Check(args)) {
    PyErr_Format(PyExc_TypeError,
        "expected an integer argument (%s given)", args->ob_type->tp_name);
    return NULL;
  }

  int i = PyInt_AsLong(args);
  if (i < 0 || i > 1) {
    PyErr_Format(PyExc_ValueError, "expected in integer 0 or 1 (%i given)",
                 i);
    return NULL;
  }

  TeamInfo info;

  Py_BEGIN_ALLOW_THREADS;
  info = self->_gc->teams(i);
  Py_END_ALLOW_THREADS;

  PyObject *t = PyTuple_New(2);
  if (t != NULL) {
    PyTuple_SET_ITEM(t, 0, PyInt_FromLong(info.teamColor));
    PyTuple_SET_ITEM(t, 1, PyInt_FromLong(info.score));
  }

  return t;
}
#endif
