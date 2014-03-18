#include "DHT.h"
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);


/* Not quite as what was planned. */
void setup() {
    Serial.begin(9600);
    Serial.println("DHTxx test!");
    dht.begin();
}

void loop() {
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(t) || isnan(h)) {
        Serial.println("Failed to read from DHT");
}
    else {
        Serial.print("Humidity:  ");
        Serial.print(h);
        Serial.print(" %\t");
        Serial.print("Temperature:  ");
        Serial.print(t);
        Serial.println(" *C");
}

    delay(2000);
} 