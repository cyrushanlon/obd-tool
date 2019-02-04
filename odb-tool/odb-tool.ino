#include "ILI9341_t3.h"
#include <OBD2UART.h>

//configuration
#define BAR_WIDTH 8
#define DISPLAY_X 320 - BAR_WIDTH
#define GRAPH_Y 200 //64
#define DISPLAY_Y 240 //64
#define TARGET_FPS 1000 / 60 //hz
#define ITEM_COUNT 4
#define BG_COLOR ILI9341_BLACK
//#define OBD_ON

//hardware
COBD obd;
#define TFT_DC  9
#define TFT_CS 10
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

int obdGetValue(int type) {
  
  int value;
  if (obd.readPID(type, value)) {
    return value;
  }

  return 0; //oh no
}

int scaleValueToScreenY(float val, float min, float max) {

  return (GRAPH_Y - 1) - ((val / max) * (GRAPH_Y - 1));
}

class item {
public:

  bool active;

  const char* name;
  float minValue;
  float maxValue;

  int pid;
  int color;

  virtual void addValue(float val);
  virtual void draw();
  item(const char* name, int pid, int color) {
    active = true;

    this->minValue = 0;
    this->maxValue = 0;

    this->name = name;
    this->pid = pid;
    this->color = color;
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
  
  graphItem(const char* name, int pid, int color) : item(name, pid, color) {

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

  barItem(const char* name, int pid, int minValue, int maxValue, int color) : item(name, pid, color) {

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
  tft.fillScreen(ILI9341_BLACK);

  //config init
  items[0] = new barItem("Throttle", PID_THROTTLE, 0, 100, ILI9341_RED);
  items[0]->maxValue = 100;
  items[1] = new graphItem("RPM", PID_RPM, ILI9341_WHITE);
  items[1]->maxValue = 9000;
  items[1]->active = true;
  items[2] = new graphItem("Oil Temp", PID_ENGINE_OIL_TEMP, ILI9341_YELLOW);
  items[2]->maxValue = 250;
  items[2]->active = false;
  items[3] = new graphItem("KMH", PID_SPEED, ILI9341_CYAN);
  items[3]->maxValue = 150;
  items[3]->active = false;

  #ifdef OBD_ON
  //obd init 
  obd.begin();
  while (!obd.init()); 
  #endif
}

unsigned long timeSinceSwitch = 0;
int current = 1;
void loop(void) {

  //display if we arent going to overshoot the target fps
  unsigned long now = millis();
  uint_fast8_t dt = now - lastFrame;
  if (dt > TARGET_FPS) {

  #ifndef OBD_ON
    Serial.println(dt);
  #endif

    //collect values from obd
    for (int i = 0; i < ITEM_COUNT; i++) {
  #ifdef OBD_ON
      int val = obdGetValue(items[i]->pid);
  #else
      int val = random(items[i]->maxValue);
  #endif
      items[i]->addValue(val);
    }
    
    tft.drawFastHLine(0, GRAPH_Y, DISPLAY_X + BAR_WIDTH, ILI9341_BLUE); // line between bottom section and graph
    for (int i = 0; i < ITEM_COUNT; i++) {
      if (items[i]->active) {
        items[i]->draw();
      }
    }

    lastFrame = now;

    //switch graph every 5 seconds
    timeSinceSwitch += dt;
    if (timeSinceSwitch > 5000) {
      items[current]->active = false;
      current++;
      if (current > ITEM_COUNT - 1) {
        current = 1;
      }
      items[current]->active = true;
      timeSinceSwitch = 0;
      tft.fillScreen(BG_COLOR);
    }
  }
}

//acceleration map
//composite item
//allow switching between graphs
//some sort of menu
