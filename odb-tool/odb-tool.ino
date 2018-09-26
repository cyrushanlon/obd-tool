#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OBD2UART.h>


//TODO
//support short term averages (5 seconds for Fuel %)
//support resetting averages
//range
//graphs
//allow for different types of items
//the below is a raw item
//need support for sub items such as MPG

struct item {
  String name; // name of the item
  float current; // current value of the item
  float max; // maximum value observed so far

  float avg;
  unsigned int avgCount;

  int PID; // PID to collect
  int dp; // number of decimal places to present the number with
};

//configuration
const bool DEBUG = true;
const int buttonPin = 2;
const int targetFPS = 3; //3 hz
const int modes = 3;
item items[3] = {
  {"RPM", 0, 0, 0, 0, PID_RPM, 0},
  {"MPH", 0, 0, 0, 0, PID_SPEED, 0},
  {"FUE", 0, 0, 0, 0, PID_FUEL_LEVEL, 0}
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
  lcd.print(i.name + " CUR MAX AVG");
  lcd.setCursor(0, 1);
  lcd.print(i.current, i.dp);
  lcd.print(" ");
  lcd.print(i.max, i.dp);
  lcd.print(" ");
  lcd.print(i.avg, i.dp);
}

void setup() {
  // init the button
  pinMode(buttonPin, INPUT);
  //init the LCD
  lcd.begin(16, 2);
  //lcd.backlight();
  lcd.setCursor(0, 0);

  if (!DEBUG) {
    lcd.print("INITIALISING");
    // start communication with OBD-II adapter
    obd.begin();
    while (!obd.init());
  } else {
    lcd.print("DEBUG MODE");
    delay(1000);
  }

  lcd.clear();

  //set the first lastFrame to avoid odd behaviour
  lastFrame = micros();
}

void loop() {

  //input
  doButton();

  //get values from obd
  for (int i = 0; i < modes; i++) {
  int val = 0;
    if (!DEBUG && obd.readPID(items[i].PID, val)) {

      items[i].current = val;
      items[i].avgCount++;
      items[i].avg = items[i].avg + ((val - items[i].avg) / items[i].avgCount);

      if (items[i].max < val) {
        items[i].max = val;
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
