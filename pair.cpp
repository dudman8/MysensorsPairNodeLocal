/*
 * Pair
 */

#include "pair.h"
#define PAIR_LED 1
#include <avdweb_Switch.h>

PairNodeLocal::PairNodeLocal(int setButtonPin, int setLedPin)
{
  //_setButtonPin = setButtonPin;
  _buttonSET = new Switch(setButtonPin);
#ifdef PAIR_LED  
  _ledPin  = setLedPin;
#endif
}

void PairNodeLocal::before() 
{
#ifdef MY_DEBUG  
  Serial.begin(115200);
  Serial.println("Starting node...");
#endif
  
  // ClearEPROM  
  _buttonSET->poll();  
  if (_buttonSET->pushed()) {
    clearEEPROM();
  }
  
  loadAllNodeState();
}

void PairNodeLocal::setup() 
{
#ifdef PAIR_LED
  pinMode(_ledPin, OUTPUT);
#endif
}

void PairNodeLocal::loop() 
{
  // Initiate _pairingMode if we're not yet
  //if (!paired) {    
    //If _pairingMode button hold for required amount of seconds initiate _pairingMode process
    _buttonSET->poll();

    if (_buttonSET->longPress() ) { // if (!digitalRead(SET_BUTTON_PIN)) {
      _paired = 0;
      Serial.println("Pair button pressed");
      pair();
      if (!_paired) {
        Serial.println("_pairingMode timeout");
      }    
    }
  //}
}

void PairNodeLocal::sendPairedMesage()
{     
  _butState = !_butState;  
  Serial.print("Button pressed paired="); Serial.println(_paired);
  //If node is paired to other node send message directly to paired node omitting controller
  if (_paired) {    
    // For each of the possible paired
    for (int i=0; i < MAX_NODE_PAIRS; i++) {
      if (_nodePair[i].freeSlot == false) {
        MyMessage msg(_nodePair[i].sensorID, V_TRIPPED);
        msg.setDestination(_nodePair[i].nodeID);
        Serial.print("Sent message to paired node<"); Serial.print(_nodePair[i].nodeID); Serial.println(">");
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
    Serial.println("Sent message to controller");
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
        Serial.print("Got pair reply from controller <"); Serial.print(message.getString()); Serial.println(">");
        int node_pair = atoi(strtok(message.getString(), ";")); //Deconstructing string from gateway, wich must contain id of paired node
        int sensor_pair = atoi(strtok(NULL, ";")); //...and id of specific sensor on that node, in case there are more than one
        Serial.print("Paired with: ");
        Serial.println(node_pair);
        Serial.println(sensor_pair);
        // add this to our registry
        int newNodeIndex = addPairedNode(node_pair, sensor_pair);
        if ( newNodeIndex != -1 ) { 
          _paired=1;
          saveNodeState(newNodeIndex);
        } else Serial.println("Node was already paired, so we didn't add it");        
      }
    }
  } // Process message sent directly from paired node
    else if (message.type == V_TRIPPED && isNodePaired(message.sender)) { //Process message sent directly from paired node
      if(isTimeUp(&timer4, _resendTime)) {
        // call external callback function thats registered on intilization of this class
        if (_pCallBack != 0) {
          Serial.println("calling callbackFunction");
          _pCallBack();        
        }
      }
    }
}

//_pairingMode function
void PairNodeLocal::pair()
{
  Serial.println("Entering _pairingMode mode");
  _pairingMode = 1;
  MyMessage pairMsg(244, V_VAR1); //I'm using specific CHILD_ID to be able to filter out _pairingMode requests
  send(pairMsg.set("Pair me."), true); //Send any message to gateway
  Serial.println("Pair request sent");
  //Then we wait some time to recieve paired node id (in my case 10 seconds)
  timer2 = millis();
  while (!isTimeUp(&timer2, _pairWaitTime)) {
    wait(1);    
    if (_paired) {
      Serial.println("Successfully recieved pairing");  printPairMap();
      break;
    } else {
#ifdef PAIR_LED  
      if (_pairingMode) flashLED();
#endif
    }
  }
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
      _nodePair[i].nodeID   == 0;
      _nodePair[i].sensorID == 0;
      _nodePair[i].freeSlot == true;
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
      Serial.println("isNodeAlreadyPaired::true");
      return true;
    }
  }
  return false;
}

bool PairNodeLocal::isNodePaired(int node_id)
{
  for (int i=0; i < MAX_NODE_PAIRS; i++) {
    if ( _nodePair[i].nodeID == node_id && _nodePair[i].freeSlot == false)  {
      Serial.print("isNodePaired::i="); Serial.println(i);
      return true;
    }
  }
  return false;    
}

bool PairNodeLocal::addPairedNode(int node_id, int sensor_id)
{
  // find an empty slot if we've not exeeded MAX_NODE_PAIRS already or we're not already paired.
  if ( isNodeAlreadyPaired(node_id, sensor_id)) {
    Serial.println("Can't add paired Node, it already is paired");
    return -1;
  }

  if (_noOfPairedNodes < MAX_NODE_PAIRS ) {
    for (int i=0; i < MAX_NODE_PAIRS; i++) {
      if ( _nodePair[i].freeSlot == true) {
        Serial.print("Found empty free space to node i="); Serial.println(i);
        _nodePair[i].nodeID = node_id;
        _nodePair[i].sensorID = sensor_id;
        _nodePair[i].freeSlot = false;
        _noOfPairedNodes ++;
        Serial.print("noOfPairedNodes="); Serial.println(_noOfPairedNodes);        
        return i;
      } 
    }
  } else {
    Serial.println("Can't add paired node, too many paired nodes registered");
    return -1;
  }
}

void PairNodeLocal::clearEEPROM() 
{
  Serial.println("Clearing EEPROM");
  
  // mysensors specific data
 // for (uint16_t i=0; i<EEPROM_LOCAL_CONFIG_ADDRESS; i++) {
//        hwWriteConfig(i,0xFF);
  //}

  // Pairing specific data
  for (int i = 0; i < MAX_NODE_PAIRS && i < 256; i++) {
    saveState(i,0);
  }
  Serial.println("EEPROM is clean");
}

// Reading _pairingMode state from EEPROM and then reading paired nodes id's if paired
void PairNodeLocal::loadAllNodeState()
{
  for (int i = 0, j = 0; i < MAX_NODE_PAIRS && i < 256; i++,j=j+3) {
    //if ( _nodePair[i].freeSlot == true) {
      _nodePair[i].nodeID == loadState(j);
      _nodePair[i].sensorID == loadState(j+1);
      _nodePair[i].freeSlot == loadState(j+2);
      // If there is a single nonFreeSlot then this node is paired to some other node
      if (_nodePair[i].freeSlot == false) {
        _paired = 1;
        _noOfPairedNodes++;
      }
    //}
  }
  printPairMap();
}

void PairNodeLocal::printPairMap()
{
  Serial.print("Paired state: "); Serial.println(_paired);
  Serial.println("table of node pairings");
   Serial.print("node:"); Serial.print("sensor:");Serial.println("freeSlot:"); 
  for (int i = 0; i < MAX_NODE_PAIRS; i++) {
    //if ( _nodePair[i].freeSlot == false) {
    Serial.print(_nodePair[i].nodeID); Serial.print(":");
    Serial.print(_nodePair[i].sensorID); Serial.print(":");
    Serial.println(_nodePair[i].freeSlot);
    //}
  }
}

// not sure if we need this function
void PairNodeLocal::saveAllNodeState()
{
  for (int i = 0, j = 0; i < MAX_NODE_PAIRS && i < 256; i++, j=j+3) {
    //if ( _nodePair[i].freeSlot == false) {
      saveState(j,   _nodePair[i].nodeID);
      saveState(j+1, _nodePair[i].sensorID);
      saveState(j+2, _nodePair[i].freeSlot);      
    //}
  }
  printPairMap();
}

// just save a change node pairing
void PairNodeLocal::saveNodeState(int i)
{
  if ( i < 256) {
    saveState(i,   _nodePair[_noOfPairedNodes].nodeID);
    saveState(i+1, _nodePair[_noOfPairedNodes].sensorID);
    saveState(i+2, _nodePair[_noOfPairedNodes].freeSlot);      
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