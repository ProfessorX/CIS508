/*
 * Always be self-motivating.
 * Life is short. Please stay awake for it.
 */



#include <SPI.h>
#include <WiFi.h>  // WiFi library


// Housekeeping
char ssid[] = "MICLOUD";
char pass[] = "";  // Open institute, open net

int status = WL_IDLE_STATUS;

// IPAddress server()
char server[] = "ec2-54-80-134-83.compute-1.amazonaws.com";

// Initialize the WiFi client library
WiFiClient client;
const unsigned long requestInterval = 30*1000;  // delay between
                                                // requests

unsigned long lastAttemptTime = 0;  // last time you connected to the
                                    // server, in milliseconds

// Variables to help in parsing XML received
String currentLine = "";
String unread = "";
boolean fountIt = false;

int led = 13;

void setup() {
    pinMode(led, OUTPUT);

    // reserve space for the strings
    currentLine.reserve(256);
    unread.reserve(50);

    // open connection to serial port for writing info to Serial
    // Monitor
    Serial.begin(9600);

    // check for the presence of the shield
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("WiFi shield not present");
        // do not continue
        while (true);
    }

    // attempt to connect to WiFi network
    // will keep on trying every 10 seconds until connected
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID:  ");
        Serial.println(ssid);
        // status = WiFi.begin(ssid, pass);  
        status = WiFi.begin(ssid);
        delay(10000);  // wait 10 seconds for connection
    }

    Serial.println("Connected to WiFi");
    printWifiStatus();
    connectToServer();


}

void loop() {
    // if there are coming bytes available 
    // from the server, read and print them
    if (client.connected()) {
        if (client.available()) {
        char c = client.read();
        // Serial.write(c);
        currentLine += c;

        if (c == '\n') {
            currentLine = "";
        }

        if (currentLine.endsWith("<fullcount>")) {
            fountIt = true;
            // break out of the loop so this character is not added to
            // <string>
            return;
        }

        if (fountIt) {
            if (c != '<') {
                unread += c;
            }
            else {
                // if you got a '<' character, you've read the number
                // indicating unread msgs. (WHY?)
                fountIt = false;
                Serial.println(unread + " unread messages.");

                // convert "unread" string to integers (aka, chars)
                char t[unread.length() + 1];
                unread.toCharArray(t, (sizeof(t)));
                int msgs = atoi(t);  // array to integer

                // blink the led 'unread' number of times
                // According to the request, the [if condition] part
                // should be modified.
                if (msgs > 0) { // this line should be modified
                    // turn the led on
                    digitalWrite(led, HIGH);
                    Serial.println("LED High");
                    delay(3000);  // Yet another delay for 3 seconds
                    // turn the led off
                    digitalWrite(led, LOW);
                    Serial.println("LED Low");
                    delay(1000);
                    msgs--;  // added so we can keep the if-else
                             // loop. Would be much clearer by using
                             // for loop
                }
            }
        }
        }
        else if (millis() - lastAttemptTime > requestInterval) {
            connectToServer();  // time out, reconnect
        }
    }

    
    

    // if the server has been disconnected, stop this poor client
    if (!client.connected()) {
        Serial.println();
        Serial.println("Disconnecting from server.");
        client.stop();

        // do nothing? do something please.
        while (true);
    }
}

void connectToServer() {
    Serial.println("\nStarting connection to server");
    // if you get a connection, report back via serial
    if (client.connect(server, 80)) {
        Serial.println("Connected to server");
        // make a HTTP request
        client.println("GET /php/get_mail.php HTTP/1.1");
        client.println("Host:ec2-54-80-134-83.compute-1.amazonaws.com");
        client.println("Connection close");
        client.println();
    }

    // note the time of this connection attempt in milliseconds
    lastAttemptTime = millis();  // this does not suck and may bring
                                 // good results
}

void printWifiStatus() {
    // print the SSID of the network you're attached to
    Serial.print("SSID:  ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address:  ");
    Serial.println(ip);

    // print the received signal strength
    long rssi = WiFi.RSSI();
    Serial.print("Signal strength (RSSI):  ");
    Serial.print(rssi);
    Serial.println("  dBm");
}

// TODO: Extract the process implemented in loop() function. Reference
// would be the Arduino IDE itself.

/*
 * Comment: From the Arduino example files there are a lot to learn.
 */