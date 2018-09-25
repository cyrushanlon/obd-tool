#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OBD2UART.h>

struct item {
  String name;
  float value;
  float max;
  int PID;
};

//configuration
const int buttonPin = 2;
const int targetFPS = 3; //3 hz
const int modes = 3;
item items[3] = {
  {"RPM", 0, 0, PID_RPM},
  {"MPH", 0, 0, PID_SPEED},
  {"Fuel", 0, 0, PID_FUEL_LEVEL}
};

//hardware
COBD obd;
LiquidCrystal_I2C lcd(0x38, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address, if it's not working try 0x27.

// vars
int ledState = HIGH;         // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

unsigned long lastFrame;
int mode;

void doButton() {

  // read the state of the switch
  int reading = digitalRead(buttonPin);

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  //if time since last bounce is longer than max delay we assume its a correct press
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == HIGH) {
        mode++;
        if (mode > modes-1) mode = 0;
      }
    }
  }

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;
}

void display() {
  item i = items[mode];

  lcd.clear();
  lcd.setCursor(0, 0);

  lcd.print(i.name + " : ");
  lcd.print(i.value);
  lcd.setCursor(0, 1);
  lcd.print("MAX : ");
  lcd.print(i.max);
}

void setup() {
  // init the button
  pinMode(buttonPin, INPUT);
  // start communication with OBD-II adapter
  obd.begin();
  //init the LCD
  lcd.begin(16, 2);
  //lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("INITIALISING");

  while (!obd.init());

  lcd.clear();

  //set the first lastFrame to avoid odd behaviour
  lastFrame = micros();
}

void loop() {

  //input
  doButton();

  //get values from obd
  for (int i = 0; i < modes; i++) {
  int value = 0;
    if (obd.readPID(items[i].PID, value)) {
      items[i].value = value;
      if (items[i].max < value) {
        items[i].max = value;
      }
    }
  }

  //display if we arent going to overshoot the target fps
  unsigned long now = millis();
  if (now - lastFrame > (1000 / targetFPS)) {
    display();
    lastFrame = now;
  }
}
