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