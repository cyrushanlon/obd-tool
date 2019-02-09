#include <ILI9341_t3.h>
#include <XPT2046_Touchscreen.h>
#include <OBD2UART.h>

//configuration
#define BAR_WIDTH 8
#define DISPLAY_X 320 - BAR_WIDTH
#define GRAPH_Y 200 //64
#define DISPLAY_Y 240 //64
#define TARGET_FPS 1000 / 60 //hz
#define ITEM_COUNT 3
#define BG_COLOR ILI9341_BLACK
//#define OBD_ON

//hardware
COBD obd;
//display
#define TFT_DC  9
#define TFT_CS 10
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);
//touchscreen
#define TOUCH_CS   8
//#define TOUCH_TIRQ 2 //optional interupt pin
XPT2046_Touchscreen ts(TOUCH_CS);//, TOUCH_TIRQ);

int scaleValueToScreenY(float val, float min, float max) {

  return (GRAPH_Y - 1) - ((val / max) * (GRAPH_Y - 1));
}

float kmhToMPH(int* pidVals) {
  return float(pidVals[0]) * 0.621371;
}

float calcMPGFromMAF(int* pidVals) {
  //MPH
  float speed = float(pidVals[0]) * 0.621371;
  //GPH
  float fuel = float(pidVals[1]) / 0.0805;
  if (fuel == 0) { //divide by 0 is bad
    fuel = 0.01;
  }
  
  return speed / fuel;
}

class item {
public:

  bool active;

  const char* name;
  float minValue;
  float maxValue;

  const int pidCount;
  int pidValues[4];
  int* pids;

  int color;

  float (*logic)(int*) = 0;

  virtual void addValue(float val);
  virtual void draw();
  item(const char* name, int color, int pids[], int pidCount) : pidCount(pidCount) {
    active = true;

    this->name = name;
    this->minValue = 0;
    this->maxValue = 0;

    this->pids = pids;

    this->color = color;
  }

  void obdGetValues() {
      
    //get results of all pids
    for (int i = 0; i < this->pidCount; i++) {
  #ifdef OBD_ON
      if (obd.readPID(this->pids[i], this->pidValues[i])) {
        //TODO: oh no one failed what do we do?
      }
  #else
      this->pidValues[i] = random(this->maxValue);
  #endif
    }
    if (this->logic) {
      this->addValue(this->logic(this->pidValues));
    } else {
      this->addValue(this->pidValues[0]);
    }
  }
};

class graphItem : public item {
public:
  
  float average;
  float currentValue;
  
  uint64_t  count;

  int graph[DISPLAY_X];
  float values[DISPLAY_X];
  float lastRender[DISPLAY_X];
  
  graphItem(const char* name, int color, int pids[], int pidCount) : item(name, color, pids, pidCount) {

    this->average = 0;
    this->count = 0;

    for (int i = 0; i < DISPLAY_X; i++) {
      this->graph[i] = GRAPH_Y;
      //this->graph[i] = GRAPH_Y * (1 + sin((float(i) / float(DISPLAY_X)) * 2 * M_PI)) / 2;
    }
  }

  void addValue(float val) {
    
    count++;
    this->average = this->average + ((val - this->average) / float(this->count));

    // move all values back and reinsert 128
    for (int i = 0; i < DISPLAY_X - 1; i++) {
      this->graph[i] = this->graph[i + 1];
      this->values[i] = this->values[i + 1];
    }
    
    //add new value
    this->currentValue = val;
    this->values[DISPLAY_X - 1] = val;

    //scale values if there is a min or max value change
    bool resize = false;
    if (val < this->minValue) { this->minValue = val; resize = true;}
    else if (val > this->maxValue) { this->maxValue = val; resize = true;}
    if (resize) {
      for (int i = 0; i < DISPLAY_X - 1; i++) {
        this->graph[i] = scaleValueToScreenY(this->values[i], this->minValue, this->maxValue);
      }
      tft.fillScreen(BG_COLOR);
    }

    this->graph[DISPLAY_X - 1] = scaleValueToScreenY(val, this->minValue, this->maxValue);
  }

  void draw() { 

    //draw first line black to erase old value and text
    // dont draw over min/max values as there is no reason to
    tft.fillRect(0, 8, 24, GRAPH_Y - 16, BG_COLOR);

    //draw graph
    for (int x = 0; x < DISPLAY_X; x++) {
      tft.drawPixel(x, this->lastRender[x - 1], BG_COLOR);  
      tft.drawPixel(x, this->graph[x], this->color);  
      lastRender[x] = graph[x];
    }
    
    //draw bottom bar values
    //name
    tft.setTextColor(ILI9341_YELLOW, BG_COLOR);
    tft.setTextSize(2);
    tft.setCursor(0, DISPLAY_Y - 20);
    tft.print(this->name);

    //values
    tft.setTextSize(1);
    tft.setCursor(150, DISPLAY_Y - 24);
    tft.print(int(this->average));
    tft.print(" ");
    tft.print(int(this->minValue));
    tft.print(" ");
    tft.print(int(this->maxValue));
    tft.print(" ");
    tft.print(millis());
    tft.print(" ");

    //min max values
    tft.setCursor(0, 0);
    tft.print(int(this->maxValue));
    tft.setCursor(0, GRAPH_Y - 8);
    tft.print(int(this->minValue));

    int y = this->graph[DISPLAY_X - 1];
    if (y < 8) {y = 8;}
    if (y > GRAPH_Y - 16) {y = GRAPH_Y - 16;}
    tft.setCursor(0, y);
    tft.print(int(this->currentValue));
  } 
};

class barItem : public item {
public:
  int height;

  barItem(const char* name, int minValue, int maxValue, int color, int pids[], int pidCount) : item(name, color, pids, pidCount) {

    this->height = 0;
    this->minValue = minValue;
    this->maxValue = maxValue;
  }

  void addValue(float val) {

    this->height = scaleValueToScreenY(100 - val, this->minValue, this->maxValue);
  }

  void draw() {

    tft.fillRect(DISPLAY_X, 0, BAR_WIDTH, GRAPH_Y, BG_COLOR);

    tft.fillRect(DISPLAY_X, GRAPH_Y - this->height, BAR_WIDTH, this->height, this->color);
  }
};

//holds item definitions
item* items[ITEM_COUNT] = {};

//times
unsigned long lastFrame = 0;

void setup(void) {

  #ifndef OBD_ON
  //connect to Serial monitor over USB
  Serial.begin(9600);
  Serial.println("START");
  #endif

  //display init
  tft.begin();  
  tft.setRotation(1);
  tft.fillScreen(ILI9341_RED);
  tft.println("INIT");

  //touch init
  tft.print("TOUCH...");
  ts.begin();
  ts.setRotation(1);
  tft.println("OK");

  //config init
  tft.print("CONFIG...");
  items[0] = new barItem("Throttle", 0, 100, ILI9341_RED, new int{PID_THROTTLE}, 1);
  items[0]->maxValue = 100;

  items[1] = new graphItem("RPM", ILI9341_YELLOW, new int{PID_RPM}, 1);
  //items[1]->maxValue = 9000;
  items[1]->active = true;
/*
  items[2] = new graphItem("Oil Temp", ILI9341_YELLOW, new int{PID_ENGINE_OIL_TEMP}, 1); //not supported on rx8
  //items[2]->maxValue = 250;
  items[2]->active = false;
*/
/*
  items[3] = new graphItem("MPH", ILI9341_YELLOW, new int{PID_SPEED}, 1);
  //items[3]->maxValue = 150;
  items[3]->logic = kmhToMPH;
  items[3]->active = false;
  items[2] = new graphItem("Fuel level", ILI9341_YELLOW, new int{PID_FUEL_LEVEL}, 1);
  //items[5]->maxValue = 100;
  items[2]->active = false;
*/

  items[2] = new graphItem("MPG", ILI9341_YELLOW, new int[2]{PID_SPEED, PID_MAF_FLOW}, 2);
  //items[4]->maxValue = 150;
  items[2]->logic = calcMPGFromMAF;
  items[2]->active = false;
/*
  items[4] = new graphItem("Fuel Usage", ILI9341_YELLOW, new int{PID_ENGINE_FUEL_RATE}, 1); //not supported on rx8
  items[4]->maxValue = 100;
  items[4]->active = false;
*/
  tft.println("OK");
  tft.print("OBD...");
  #ifdef OBD_ON
  delay(2000);
  //obd init 
  obd.begin();
  while (!obd.init()); 
  #endif
  tft.println("OK");
  tft.setCursor(0,0);
  tft.fillScreen(BG_COLOR);
}

unsigned long timeSinceSwitch = 0;
int current = 1;
void loop(void) {

  //display if we arent going to overshoot the target fps
  uint_fast8_t dt = millis() - lastFrame;
  if (dt > TARGET_FPS) {

  #ifndef OBD_ON
    Serial.println(dt);
  #endif

    //collect values from obd
    for (int i = 0; i < ITEM_COUNT; i++) {
      items[i]->obdGetValues();
    }
    
    tft.drawFastHLine(0, GRAPH_Y, DISPLAY_X + BAR_WIDTH, ILI9341_BLUE); // line between bottom section and graph
    for (int i = 0; i < ITEM_COUNT; i++) {
      if (items[i]->active) {
        items[i]->draw();
      }
    }

    //switch graph with touch
    timeSinceSwitch += dt;
    if (timeSinceSwitch > 500 && ts.touched()) { //500 ms debounce time
      items[current]->active = false;
      current++;
      if (current > ITEM_COUNT - 1) {
        current = 1;
      }
      items[current]->active = true;
      timeSinceSwitch = 0;
      tft.fillScreen(BG_COLOR);
    }

    lastFrame = millis();
  }
}

//acceleration map
//some sort of menu
