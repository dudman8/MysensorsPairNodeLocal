/*
 * Pair
 */

#include "pair.h"
#define PAIR_LED 1

#include <Button.h>

/******************************************
    Request
*/

Request::Request(const char* string) {
  char str[10];
  char* ptr;
  strcpy(str,string);
  // tokenize the string and split function from value
  strtok_r(str,",",&ptr);
  _function = atoi(str);
  strcpy(_value,ptr);

    DEBUG_PRINT(F("REQ F="));
    DEBUG_PRINT(getFunction());
    DEBUG_PRINT(F(" I="));
    DEBUG_PRINT(getValueInt());
    DEBUG_PRINT(F(" F="));
    DEBUG_PRINT(getValueFloat());
    DEBUG_PRINT(F(" S="));
    DEBUG_PRINTLN(getValueString());

}

// return the parsed function
int Request::getFunction() {
  return _function;
}

// return the value as an int
int Request::getValueInt() {
  return atoi(_value);
  
}

// return the value as a float
float Request::getValueFloat() {
  return atof(_value);
}

// return the value as a string
char* Request::getValueString() {
  return _value;
}

PairNodeLocal::PairNodeLocal(int setButtonPin, int setLEDPin, int cathodeORAnode)
{
//PairNodeLocal::PairNodeLocal(int setButtonPin, int setLedPin)
//{
  //_setButtonPin = setButtonPin;
  //_buttonSET = new Switch(setButtonPin);
  _buttonSET = new Button(setButtonPin, true, true, 20);
#ifdef PAIR_LED  
  _ledPin  = setLEDPin;
  _ledType = cathodeORAnode;
#endif
}

void PairNodeLocal::setLedPin(int pin, int cathodeORAnode)
{

  _ledPin  = pin;
  _ledType = cathodeORAnode;
}

void PairNodeLocal::before() 
{
#ifdef MY_DEBUG  
  Serial.begin(115200);
  DEBUG_PRINTLN("Starting node...");
#endif
  
  // ClearEPROM  
  //_buttonSET->poll();  
  _buttonSET->read();
  //if (_buttonSET->longPress()) {
  if (_buttonSET->isPressed()) {
    clearEEPROM();
  }
  
  loadAllNodeState();
}

void PairNodeLocal::setup() 
{
#ifdef PAIR_LED
  pinMode(_ledPin, OUTPUT);    
  // Ensure led is off
  digitalWrite(_ledPin, !_ledType); // Inverse of _ledType is the LOW value
#endif
}

void PairNodeLocal::loop() 
{
  // Initiate _pairingMode if we're not yet
  //if (!paired) {    
    //If _pairingMode button hold for required amount of seconds initiate _pairingMode process
    _buttonSET->read();

    if (_buttonSET->pressedFor(2000) ) { // if (!digitalRead(SET_BUTTON_PIN)) {
      _paired = 0;
      DEBUG_PRINTLN("Pair button pressed");
      pair();
      if (!_paired) {
        DEBUG_PRINTLN("_pairingMode timeout");
      }    
    }
  //}
}

void PairNodeLocal::sendPairedMesage()
{     
  _butState = !_butState;  
  DEBUG_PRINT("Button pressed paired="); DEBUG_PRINTLN(_paired);
  //If node is paired to other node send message directly to paired node omitting controller
  if (_paired) {    
    // For each of the possible paired
    for (int i=0; i < MAX_NODE_PAIRS; i++) {
      if (_nodePair[i].freeSlot == false) {
        MyMessage msg(_nodePair[i].sensorID, V_TRIPPED);
        msg.setDestination(_nodePair[i].nodeID);
        DEBUG_PRINT("Sent message to paired node<"); DEBUG_PRINT(_nodePair[i].nodeID); DEBUG_PRINTLN(">");
        int retry = 0;
        while(!send(msg.set(1)) || retry == 10) {
          wait(100);
          send(msg.set(1));
          retry++;
        }
      }
    }
  } else { //If not, send message to controller      
    MyMessage msg(CHILD_ID, V_TRIPPED);
    send(msg.set(1));
    DEBUG_PRINTLN("Sent message to controller");
  }      
}

void PairNodeLocal::receive(const MyMessage &message)
{  
  // While in _pairingMode mode we'll only process _pairingMode messages
  if (_pairingMode) {
    if (!message.sender) {
      // Expand messages processed 
      // 1) pair reply fron controller
      // 2) pair removeal from controller or node
      // 3) list pairedNodes
      if (message.type == V_VAR2) {
        DEBUG_PRINT("Got pair reply from controller <"); DEBUG_PRINT(message.getString()); DEBUG_PRINTLN(">");
        int node_pair = atoi(strtok(message.getString(), ";")); //Deconstructing string from gateway, wich must contain id of paired node
        int sensor_pair = atoi(strtok(NULL, ";")); //...and id of specific sensor on that node, in case there are more than one
        DEBUG_PRINT("Paired with: ");DEBUG_PRINT(node_pair);DEBUG_PRINT("-"); DEBUG_PRINTLN(sensor_pair);
        // add this to our registry
        int newNodeIndex = addPairedNode(node_pair, sensor_pair);
        if ( newNodeIndex != -1 ) { 
          _paired=1;
          saveNodeState(newNodeIndex);
          //saveAllNodeState();
        } else DEBUG_PRINTLN("Node was already paired, so we didn't add it");        
      }
    }
  } // Process message sent directly from paired node
    else if (message.type == V_TRIPPED && isNodePaired(message.sender)) { //Process message sent directly from paired node
      if(isTimeUp(&timer4, _resendTime)) {
        // call external callback function thats registered on intilization of this class
        if (_pCallBack != 0) {
          DEBUG_PRINTLN("calling callbackFunction");
          _pCallBack();        
        }
      }
    }
}

//_pairingMode function
void PairNodeLocal::pair()
{
  DEBUG_PRINTLN("Entering _pairingMode mode");
  _pairingMode = 1;
  MyMessage pairMsg(244, V_VAR1); //I'm using specific CHILD_ID to be able to filter out _pairingMode requests
  send(pairMsg.set("Pair me."), true); //Send any message to gateway
  DEBUG_PRINTLN("Pair request sent");
  //Then we wait some time to recieve paired node id (in my case 10 seconds)
  timer2 = millis();
  while (!isTimeUp(&timer2, _pairWaitTime)) {
    wait(1);    
    if (_paired) {
      DEBUG_PRINTLN("Successfully recieved pairing");  printPairMap();
      break;
    } else {
#ifdef PAIR_LED  
      if (_pairingMode) flashLED();
#endif
    }
  }
#ifdef PAIR_LED  
  // Ensure the flashing led finishes OFF.
  digitalWrite(_ledPin, !_ledType); // Inverse of _ledType is the LOW value
#endif
  _pairingMode = 0;
}

#ifdef PAIR_LED  
void PairNodeLocal::flashLED()
{
  // Flash pairingLED
  // check to see if it's time to blink the LED; that is, if the difference
  // between the current time and last time you blinked the LED is bigger than
  // the interval at which you want to blink the LED.
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillisA >= interval) {
    // save the last time you blinked the LED
    previousMillisA = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledStateA == LOW) {
      ledStateA = HIGH;
    } else {
      ledStateA = LOW;
    }
    // set the LED with the ledState of the variable:
    digitalWrite(_ledPin, ledStateA);
  }
}
#endif

bool PairNodeLocal::removePairedNode(int node_id, int sensor_id)
{
 for (int i=0; i < MAX_NODE_PAIRS; i++) {
    if ( _nodePair[i].nodeID == node_id && _nodePair[i].sensorID == node_id) {
      _noOfPairedNodes --;
      _nodePair[i].nodeID   = 0;
      _nodePair[i].sensorID = 0;
      _nodePair[i].freeSlot = true;
      printPairMap();
      return true;
    }
  }
  return false; 
}

bool PairNodeLocal::isNodeAlreadyPaired(int node_id, int sensor_id)
{
  for (int i=0; i < MAX_NODE_PAIRS; i++) {
    if ( _nodePair[i].nodeID == node_id && 
         _nodePair[i].sensorID == sensor_id && 
        _nodePair[i].freeSlot == false)  {
      DEBUG_PRINTLN("isNodeAlreadyPaired::true");
      return true;
    }
  }
  return false;
}

bool PairNodeLocal::isNodePaired(int node_id)
{
  for (int i=0; i < MAX_NODE_PAIRS; i++) {
    if ( _nodePair[i].nodeID == node_id && _nodePair[i].freeSlot == false)  {
      DEBUG_PRINT("isNodePaired::i="); DEBUG_PRINTLN(i);
      return true;
    }
  }
  return false;    
}

bool PairNodeLocal::addPairedNode(int node_id, int sensor_id)
{
  // find an empty slot if we've not exeeded MAX_NODE_PAIRS already or node is not already paired.
  if ( isNodeAlreadyPaired(node_id, sensor_id)) {
    DEBUG_PRINTLN("Can't add paired Node, it already is paired");
    return -1;
  }

  if (_noOfPairedNodes < MAX_NODE_PAIRS ) {
    for (int i=0; i < MAX_NODE_PAIRS; i++) {
      if ( _nodePair[i].freeSlot == true) {
        DEBUG_PRINT("Found empty free space to node i="); DEBUG_PRINTLN(i);
        _nodePair[i].nodeID = node_id;
        _nodePair[i].sensorID = sensor_id;
        _nodePair[i].freeSlot = false;
        _noOfPairedNodes ++;
        DEBUG_PRINT("noOfPairedNodes="); DEBUG_PRINTLN(_noOfPairedNodes);        
        return i;
      } 
    }
  } else {
    DEBUG_PRINTLN("Can't add paired node, too many paired nodes registered");
    return -1;
  }
}

void PairNodeLocal::clearEEPROM() 
{
  DEBUG_PRINTLN("Clearing EEPROM");
  
  // mysensors specific data
 // for (uint16_t i=0; i<EEPROM_LOCAL_CONFIG_ADDRESS; i++) {
//        hwWriteConfig(i,0xFF);
  //}

  // Pairing specific data
  for (int i = 0, j = 0; i < MAX_NODE_PAIRS && i < 256; i++,j=j+3) {
    saveState(j,0);
    saveState(j+1,0);
    saveState(j+2,1);
  }
  DEBUG_PRINTLN("EEPROM is clean");
}

// Reading _pairingMode state from EEPROM and then reading paired nodes id's if paired
void PairNodeLocal::loadAllNodeState()
{
  DEBUG_PRINTLN("::loadAllNodeState");
  for (int i = 0, j = 0; i < MAX_NODE_PAIRS && i+2 < 256; i++,j=j+3) {
    //if ( _nodePair[i].freeSlot == true) {
      _nodePair[i].nodeID = loadState(j);
      _nodePair[i].sensorID = loadState(j+1);
      _nodePair[i].freeSlot = loadState(j+2);    
      DEBUG_PRINT(_nodePair[i].nodeID); DEBUG_PRINT(":");
      DEBUG_PRINT(_nodePair[i].sensorID); DEBUG_PRINT(":");
      DEBUG_PRINTLN(_nodePair[i].freeSlot);
      // If there is a single nonFreeSlot then this node is paired to some other node
      if (_nodePair[i].freeSlot == false) {
        _paired = 1;
        _noOfPairedNodes++;
      }
    //}
  }  
  printPairMap();
  printEEPROMMap();
}

void PairNodeLocal::printEEPROMMap()
{
  DEBUG_PRINT("::printEEPROMMAP,");
  DEBUG_PRINT("Paired state: "); DEBUG_PRINTLN(_paired);
  DEBUG_PRINT("table of node direct from eeprom<"); DEBUG_PRINT(getNodeId()); DEBUG_PRINTLN("> pairings");
   DEBUG_PRINT("node:"); DEBUG_PRINT("sensor:");DEBUG_PRINTLN("freeSlot:"); 
   for (int i = 0, j = 0; i < MAX_NODE_PAIRS && i+2 < 256; i++,j=j+3) {   
      int nodeID = loadState(j);
      int sensorID = loadState(j+1);
      int freeSlot = loadState(j+2);
     // DEBUG_PRINT("i="); DEBUG_PRINT(i); DEBUG_PRINT("j=");DEBUG_PRINTLN(j);
      DEBUG_PRINT(nodeID); DEBUG_PRINT(":");
      DEBUG_PRINT(sensorID); DEBUG_PRINT(":");
      DEBUG_PRINTLN(freeSlot);
  } 
}

// just save a change node pairing
void PairNodeLocal::saveNodeState(int i)
{
  if ( i < 256) {
    DEBUG_PRINT("::savelNodeState:i="); DEBUG_PRINTLN(i);
    DEBUG_PRINT(_nodePair[i].nodeID); DEBUG_PRINT(":");
    DEBUG_PRINT(_nodePair[i].sensorID); DEBUG_PRINT(":");
    DEBUG_PRINTLN(_nodePair[i].freeSlot);
    saveState(i,   _nodePair[i].nodeID);
    saveState(i+1, _nodePair[i].sensorID);
    saveState(i+2, _nodePair[i].freeSlot);      
  }
  printPairMap();
  printEEPROMMap();
}

// not sure if we need this function
void PairNodeLocal::saveAllNodeState()
{
  DEBUG_PRINT("::printEEPROMMAP,");
  for (int i = 0, j = 0; i < MAX_NODE_PAIRS && i < 256; i++, j=j+3) {
    //if ( _nodePair[i].freeSlot == false) {
      saveState(j,   _nodePair[i].nodeID);
      saveState(j+1, _nodePair[i].sensorID);
      saveState(j+2, _nodePair[i].freeSlot);    
     // DEBUG_PRINT("i="); DEBUG_PRINT(i); DEBUG_PRINT("j=");DEBUG_PRINTLN(j);
      DEBUG_PRINT(_nodePair[i].nodeID); DEBUG_PRINT(":");
      DEBUG_PRINT(_nodePair[i].sensorID); DEBUG_PRINT(":");
      DEBUG_PRINTLN(_nodePair[i].freeSlot);  
    //}
  }
  printPairMap();
  printEEPROMMap();
}

void PairNodeLocal::printPairMap()
{
  DEBUG_PRINT("Paired state: paired="); DEBUG_PRINT(_paired); DEBUG_PRINT("_noOfPairedNodes="); DEBUG_PRINTLN(_noOfPairedNodes);
  DEBUG_PRINT("table of node <"); DEBUG_PRINT(getNodeId()); DEBUG_PRINTLN("> pairings");
   DEBUG_PRINT("node:"); DEBUG_PRINT("sensor:");DEBUG_PRINTLN("freeSlot:"); 
  for (int i = 0; i < MAX_NODE_PAIRS; i++) {
    //if ( _nodePair[i].freeSlot == false) {
    DEBUG_PRINT(_nodePair[i].nodeID); DEBUG_PRINT(":");
    DEBUG_PRINT(_nodePair[i].sensorID); DEBUG_PRINT(":");
    DEBUG_PRINTLN(_nodePair[i].freeSlot);
    //}
  }
}

//Time counter function instead of delay
boolean PairNodeLocal::isTimeUp(unsigned long *timeMark, unsigned long timeInterval) {
    if (millis() - *timeMark >= timeInterval) {
        *timeMark = millis();
        return true;
    }
    return false;
}

void PairNodeLocal::setPairedMessageRecievedFunction(void (*pCallbackFunction)(void))
{
  _pCallBack = pCallbackFunction;
}