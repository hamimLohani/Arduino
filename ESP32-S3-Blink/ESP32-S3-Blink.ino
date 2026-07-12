#define DEFAULT_LED 48

void setup() {
  // No pinmode initialization is strictly needed for rgbLedWrite
}

void loop() {
  // Blink Bright Green: rgbLedWrite(pin, red, green, blue)
  rgbLedWrite(DEFAULT_LED, 0, 255, 0); 
  delay(500);
  
  // Turn OFF
  rgbLedWrite(DEFAULT_LED, 0, 0, 0);   
  delay(500);
}