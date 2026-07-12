#include "DHT.h"

#define PIN 12
#define DHTPIN 2        // Connect DHT11 data pin to D2
#define DHTTYPE DHT11   // Define sensor type

DHT dht(DHTPIN, DHTTYPE);  // Create DHT object

void setup() {
  Serial.begin(9600);      // Start serial monitor
  dht.begin();             // Initialize the sensor
  pinMode(PIN, OUTPUT); 
}

void loop() {
  float temp = dht.readTemperature();     // Read temperature in °C
  float hum = dht.readHumidity();         // Read humidity %

  if (isnan(temp) || isnan(hum)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  if (hum > 70) {
    Serial.println("**Hi Humidity**");
    digitalWrite(PIN, HIGH);
  } else digitalWrite(PIN, LOW);

  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.println(" %");

  delay(2000);  // Wait 2 seconds between reads
}