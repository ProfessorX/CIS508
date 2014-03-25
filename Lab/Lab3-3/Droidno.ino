#include <Max3421e.h>
#include <Usb.h>
#include <AndroidAccessory.h>

// Housekeeping
// Macros
#define COMMAND_SENSOR 0xF
#define COMMAND_LED 0x2
#define LED_OFF 0x0
#define LED_ON  0x1
#define LED_PIN 13

byte rcvmsg[2];  // received message is two bytes long
byte sntmsg[5];  // sent message is 5 bytes long
int sensorValue = 100;  // some dummy sensor

AndroidAccessory acc("yxiao",  // manufacturer
                     "lab3.3",  // model
                     "Basic ADK Sketch",  // description
                     "1.0",  // version
                     "http://abrahamx.com",
                     "0000000012345678");

void setup() {
    Serial.begin(115200);  // Baud rate
    pinMode(LED_PIN, OUTPUT);
    acc.powerOn();  // start the android app
}

void loop() {
    
}