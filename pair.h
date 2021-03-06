#ifndef pair_h
#define pair_h


// turn debug on off, comment below
#define DEBUG 1

#ifdef DEBUG
 #define DEBUG_PRINT(x)  Serial.print (x)
 #define DEBUG_PRINTLN(x)  Serial.println(x)
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTLN(x)
#endif

#include <core/MySensorsCore.h>
#include <Button.h> //https://github.com/JChristensen/Button

/**********************
 * 
 *
 */

#define CHILD_ID 3

// if defined then insert additional functionality for flashing
// of pairing LED
#define PAIR_LED 1

// Handle multiply pairing between nodes
#define MAX_NODE_PAIRS 5
typedef struct NodePair {
  byte nodeID   = 0;
  byte sensorID = 0;
  bool freeSlot = true; // this slot in the array can be reused.
};

/*
   Request
*/

class Request {
  public:
    Request(const char* string);
    // return the parsed function
    int getFunction();
    // return the value as an int
    int getValueInt();
    // return the value as a float
    float getValueFloat();
    // return the value as a string
    char* getValueString();
   private:
    //NodeManager* _node_manager;
    int _function;
    char* _value;
};

#define LEDCATHODE  1   // its HIGH value
#define LEDANODE    0   // Its HIGH value

class PairNodeLocal 
{
public:
  PairNodeLocal(int setButtonPin, int setLEDPin = -1, int cathodeORAnode = LEDCATHODE);
  PairNodeLocal(int setButtonPin);
  void setLedPin(int pin, int cathodeORAnode);  
  void before();
  void setup();
  void loop();
  void receive(const MyMessage &message);
  void sendPairedMesage(); // for a paired node to trigger a pair message to be sent to its paired nodes
  void setPairedMessageRecievedFunction(void (*pCallbackFunction)(void)); // for a slave to do something when a message is sent from a master
  bool isPaired(){ return _paired;}

private:  
  void pair();  

  // managed paired status in EEPROM
  void clearEEPROM();
  void loadAllNodeState();
  void saveAllNodeState();
  void saveNodeState(int index);

  // helper functions to manager the array of pairings.
  bool removePairedNode(int node_id, int sensor_id);
  bool isNodeAlreadyPaired(int node_id, int sensor_id);
  bool addPairedNode(int node_id, int sensor_id);
  bool isNodePaired(int node_id);
  void printPairMap();
  void printEEPROMMap();

  void flashLED();
  boolean isTimeUp(unsigned long *timeMark, unsigned long timeInterval);

  // Are we in the middle of pairing  ?
  bool _pairingMode    = 0;
  
  // Array of pairing between nodes
  NodePair _nodePair[MAX_NODE_PAIRS];
  int _noOfPairedNodes = 0;
  
  // Is this node paired ?
  bool _paired     = 0;
  int _pairWaitTime= 10000;

  unsigned long timer2 = 0;
  
  bool _butState   = 0;
  
  // used when ensuring message is sent
  unsigned long timer4;
  int _resendTime = 1500;

  Button *_buttonSET = 0;

  // function to call when a message is recieved from a paired node
  void (*_pCallBack)(void);

#ifdef PAIR_LED
  int ledStateA = LOW;
  int _ledPin = -1;
  int _ledType = LEDCATHODE;
  unsigned long previousMillisA = 0;        // will store last time LED was updated
  // constants won't change:
  const long interval = 200;
#endif
};


#endif