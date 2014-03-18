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

}

void loop() {
    // if there are coming bytes available 
    // from the server, read and print them
    

}