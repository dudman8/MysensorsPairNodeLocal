/*
 * Example of relay node which can pair with directly with a Master node so control dosn't have to be dependant upon
 * the reliability of the controller. 
 * Pairing is started by 
 *
 * This has been addapted from origional code from monte forum user at https://forum.mysensors.org/topic/8716/direct-pairing-of-two-nodes-implementation
 */

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RFM69 
#define MY_RFM69_FREQUENCY RFM69_433MHZ

#include <MySensors.h>

#define CHILD_ID 1
#define RELAY_PIN  5  // Arduino Digital I/O pin number for relay
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay

bool relayState;

MyMessage msg(CHILD_ID,V_LIGHT);

#include <Button.h>
#include <pair.h>

#define SET_BUTTON_PIN 4
#define SET_LED_PIN 6    // led indicator that will flash when in pairing mode, comment out to disable
#define SET_LED_PIN 6    // led indicator that will flash when in pairing mode, comment out to disable
PairNodeLocal pairLocal = PairNodeLocal(SET_BUTTON_PIN, 6, LEDANODE);

// I think send to controller the button regardless if its paired directly 
void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Relay", "1.0");

  present(CHILD_ID, S_LIGHT, "Test light", true);

  // Send saved state to gateway (using eeprom storage)
 // send(msg.set(relayState),true);
}

void before()
{
  pairLocal.before();

  // Setup the relay
  pinMode(RELAY_PIN, OUTPUT);
  //digitalWrite(RELAY_PIN, 1);
  //delay(1000);
  //digitalWrite(RELAY_PIN, 0);

  // Set relay to last known state (using eeprom storage)
  //relayState = loadState(CHILD_ID);
  //digitalWrite(RELAY_PIN, relayState ? RELAY_ON : RELAY_OFF);
  digitalWrite(RELAY_PIN, 1);
}

void setup()
{
  pairLocal.setup();

  //optionally pass function in, Relay change state
  uint16_t ptr = &ChangeRelay;
  pairLocal.setPairedMessageRecievedFunction(ptr);
}

void loop()
{
  pairLocal.loop();
}

void receive(const MyMessage &message)
{
  pairLocal.receive(message);

  // Additional process message from Gateway
  if (message.type == V_LIGHT && !message.sender) {
      // Change relay state
      relayState = message.getBool();
      digitalWrite(RELAY_PIN, relayState ? RELAY_ON : RELAY_OFF);
      // Store state in eeprom
      //saveState(CHILD_ID, relayState);
      // Write some debug info
      Serial.print("Incoming change. New status:");
      Serial.println(relayState);
    } 
}

// Function called when a paired master node sends a message
void ChangeRelay()
{
	Serial.println("Relay changed state by master node");
	digitalWrite(RELAY_PIN, relayState ? RELAY_OFF : RELAY_ON);
    relayState = relayState ? 0 : 1;
  Serial.println("changed relay");

    // Save state of relay here if you want
}