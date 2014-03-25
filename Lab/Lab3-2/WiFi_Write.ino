/*
 * Is life hard at research institute? I have to say so. But you
 * also have to learn to adapt to the world.
 */


// Libraries
#include <SPI.h>
#include <WiFi.h>  // WiFi library
#include "DHT.h"

// Macros
#define DHTPIN 2
#define DHTTYPE DHT22

// Housekeeping
char ssid[] = "MICLOUD";
char pass[] = "";

int status = WL_IDLE_STATUS;
char server[] = "ec2-54-80-134-83.compute-1.amazonaws.com";

// Initialize the WiFi client library
WiFiClient client;
DHT dht(DHTPIN, DHTTYPE);
double dummyValue = 1989.64;
float TEMP_READING = 0.0;


void setup() {
    Serial.begin(9600);

    // check for the presence of the shield
    if (WiFi.status == WL_NO_SHIELD) {
        Serial.println("WiFi shield not present");
        // do not continue
        while (true);  // loop forever here? It seems so.
    }

    // attempt to connect to WiFi network:
    // will keep trying every 10 seconds until connected
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID:  ");
        Serial.println(ssid);
        // status - WiFi.begin(ssid, pass);  // in this case you have
        // to use the password to connect
        status = WiFi.begin(ssid);
        delay(10000);  // wait for 10 seconds for connection to
                       // establish
    }
    Serial.println("Connected to WiFi");
    printWifiStatus();
}

void loop() {
    // connect to the server
    if (client.connect(server, 80)) {
        // print to serial monitor
        Serial.println("Connection established!");
        Serial.println("Arduino: forming HTTP request messages...");

        // send data to the server through GET request
        client.print("GET /php/sensor_db.php?sensortype=temperature&reading=");       
        /* client.print("GET /php/sensor_view.php?sensortype=Temp&reading="); */
        client.print(readTemperature());
        client.println(" HTTP/1.1");
        client.println("Host: ec2-54-82-125-42.compute-1.amazonaws.com");
        client.println();

        Serial.println("Arduino: HTTP message sent");

        // give server some time to receive and store data
        // before asking for response from it
        delay(1000);

        if (client.available()) {
            Serial.println("Arduino: HTTP message received.");
            Serial.println("Arduino: printing received hearders and script response...\n");

            while (client.available()) {
                char c = client.read();
                Serial.print(c);
            }
        }
        else {
            Serial.println("Arduino: no response received / no response received in time");
        }


    }
    else if (millis() - lastAttemptTime > requestInterval){
        connectToServer();  // time out, reconnect
    }
    else {
        ;  // do nothing here
    }

    // do nothing, just loop forever
    /* while (true); */
}

float readTemperature() {
    TEMP_READING = dht.readTemperature();
    return TEMP_READING;

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