#include <LiquidCrystal_I2C.h>

// I2C address 0x27
LiquidCrystal_I2C lcd(0x27, 16, 2); 

void setup() {
  Wire.begin(21, 20); // ESP32 I2C pins: SDA = 21, SCL = 22
  lcd.init();         // Must use .init() not .begin()
  lcd.backlight();    

  lcd.setCursor(0, 0);
  lcd.print("ESP32 + LCD");

  lcd.setCursor(0, 1);
  lcd.print("Working Bro");
}

void loop() {
  // Nothing needed in loop for now
}