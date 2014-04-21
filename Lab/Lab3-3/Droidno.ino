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
    if (acc.isConnected()) {
        // read the 2-byte message into the byte array
        int len = acc.read(rcvmsg, sizeof(rcvmsg), 1);
        
        if (len > 0) {
            if (rcvmsg[0] == COMMAND_LED) {
                byte value = rcvmsg[1];  // second byte is the actual
                                         // command sent from phone
                if (value == LED_ON) {  // if ON command was sent
                    digitalWrite(LED_PIN, HIGH);
                }
                else if (value == LED_OFF) {
                    digitalWrite(LED_PIN, LOW);
                }
            }
        }
        
        // Sample the sensor and send the reading to phone. We'll send
        // 4-byte integers to demonstrate how to send more than 1-byte
        // values. We use bit-shift operations.
        sntmsg[0] = COMMAND_SENSOR;
        sntmsg[1] = (byte) (sensorValue >> 24);
        sntmsg[2] = (byte) (sensorValue >> 16);
        sntmsg[3] = (byte) (sensorValue >> 8);
        sntmsg[4] = (byte) (sensorValue);

        // send the constructed 5-byte message to the phone
        acc.write(sntmsg, 5);

        // arbitrarily change dummy sensor in each iteration of loop
        // so that sensor readings can be observed changing in
        // real-time through Android APPs.
        sensorValue += 10;
        if (sensorValue > 500) {
            sensorValue = 100;  // reset
        }
    }
}