#include "USB.h"
#include "USBCDC.h"

USBCDC USBSerial;

void setup() {
  Serial.begin(115200);    // COM port (UART0)
  USBSerial.begin(115200); // USB port (Native USB)
  USB.begin();
  
  delay(1000);
  Serial.println("Hello via COM port!");
  USBSerial.println("Hello via USB port!");
}

void loop() {
  Serial.println("Running...");
  USBSerial.println("usbRunning...");
  delay(1000);
}