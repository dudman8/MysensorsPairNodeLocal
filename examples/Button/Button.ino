/*
 * Description on node
 */

// Enable debug prints to serial monitor
#define MY_DEBUG
#define MY_NODE_ID 100

// Enable and select radio type attached
#define MY_RADIO_RFM69 
#define MY_RFM69_FREQUENCY RFM69_433MHZ

#include <MySensors.h>

#define CHILD_ID 3

#include <Button.h> //https://github.com/JChristensen/Button
#include <pair.h>

#define SET_BUTTON_PIN 5 // Arduino Digital I/O pin for button/reed switch
#define SET_LED_PIN 6    // led indicator that will flash when in pairing mode, comment out to disable
#define BUTTON_PIN 4  // Normal button PIN
Button button = Button(BUTTON_PIN, true, true, 50);

/* Class which handles all the pairing between nodes, you need to add methods at the begining of 
 * before(), setup(), loop() and recieve() eg pairLocal.before() etc.
 * Also define and set the callback function setPairedMessageRecievedFunction() which you want to be called when this node recieves messages
 * from its paired master or masters.
 *
 */
PairNodeLocal pairLocal = PairNodeLocal(SET_BUTTON_PIN, SET_LED_PIN,LEDANODE);


// I think send to controller the button regardless if its paired directly 
void presentation()
{
  // Send the sketch version information to the Controller in case node is'nt paired
  if (!pairLocal.isPaired()) {
    sendSketchInfo("Binary paired button", "1.0");
    present(CHILD_ID, S_MOTION);
  }
}

void before()
{
  pairLocal.before();

  //optionally pass function in, This button node won't recieve messages to do something from a slave
  uint16_t ptr = &ChangeRelay; // ??? I don't know if this is the way to do this ???
  pairLocal.setPairedMessageRecievedFunction(ptr);
}

void setup()
{
  pairLocal.setup();
}

void loop()
{
  pairLocal.loop();

  // Process our own sensors additional button, send messages to any paired nodes
  button.read();
  // button was pushed
  if (button.wasPressed()) {   
    if (pairLocal.isPaired()) 
      pairLocal.sendPairedMesage();
    else {
      MyMessage msg(CHILD_ID, V_TRIPPED);
      send(msg.set(0)); // NOT SURE IF WE NEED THIS ????
    }
  } 
}


void receive(const MyMessage &message)
{
  pairLocal.receive(message);

  // Do any additional processing here.
}

// Callback Function called when a paired master node sends a message
void ChangeRelay()
{

}