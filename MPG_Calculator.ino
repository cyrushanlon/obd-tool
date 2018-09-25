#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OBD.h>

COBDI2C obd;

LiquidCrystal_I2C lcd(0x38, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address, if it's not working try 0x27.

void setup() {
  // start communication with OBD-II adapter
  obd.begin();
  //init the LCD
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("INITIALISING");

  while (!obd.init());

  lcd.setCursor(0, 0);
  lcd.print("RPM:");
}

void loop() {
  int value;
  // save engine RPM in variable 'value', return true on success
  if (obd.readPID(PID_RPM, value)) {
    // light on LED on Arduino board when the RPM exceeds 3000
    lcd.setCursor(0, 1);
    lcd.print(value);
  }
}
