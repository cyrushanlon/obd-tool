//configuration
#define BAR_WIDTH 4
#define DISPLAY_X 128 - BAR_WIDTH
#define GRAPH_Y 55 //64
#define DISPLAY_Y 64 //64
#define TARGET_FPS 1000 / 60 //hz
#define ITEM_COUNT 2
//#define OBD_ON

#include <U8g2lib.h>
#include <OBD2UART.h>

//hardware
COBD obd;
U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R2, 10, 7, 8); //rotation, cs, dc [, reset]

int obdGetValue(byte type) {
  
  int value;
  if (obd.readPID(type, value)) {
    return value;
  }

  return 0; //oh no
}

int scaleValueToScreenY(float val, float min, float max) {

  return GRAPH_Y - ((val / max) * GRAPH_Y);
}

class item {
public:

  bool active;

  const char* name;
  float minValue;
  float maxValue;

  int pid;

  virtual void addValue(float val);
  virtual void draw();
  item(const char* name, int pid) {
    active = true;

    this->minValue = 0;
    this->maxValue = 0;

    this->name = name;
    this->pid = pid;
  }
};

class graphItem : public item {
public:
  
  float average;
  float currentValue;
  
  uint64_t  count;

  int graph[DISPLAY_X];
  float values[DISPLAY_X];
  
  graphItem(const char* name, int pid) : item(name, pid) {

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
    //draw graph
    for (int x = 0; x < DISPLAY_X; x++) {
      u8g2.drawPixel(x, this->graph[x]);  
    }

    //draw bottom bar values
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.setCursor(0, DISPLAY_Y);
    //u8g2.print(u8x8_u16toa(this->currentValue, 4));
    //u8g2.print(" ");
    u8g2.print(u8x8_u16toa(this->average, 4));
    u8g2.print(" ");
    u8g2.print(int(this->minValue));
    u8g2.print(" ");
    u8g2.print(int(this->maxValue));
    u8g2.print(" ");
    u8g2.print(millis());
    u8g2.print(" ");

    //min max values
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(0, 7);
    u8g2.print(int(this->maxValue));
    int y = this->graph[DISPLAY_X - 1];
    if (y < 14) {y = 14;}
    u8g2.setCursor(0, y);
    u8g2.print(int(this->currentValue));
    u8g2.print(" ");   
  } 
};

class barItem : public item {
public:
  int height;

  barItem(const char* name, int pid, int minValue, int maxValue) : item(name, pid){

    this->height = 0;
    this->minValue = minValue;
    this->maxValue = maxValue;
  }

  void addValue(float val) {

    this->height = scaleValueToScreenY(100 - val, this->minValue, this->maxValue);
  }

  void draw() {

    u8g2.drawBox(DISPLAY_X, GRAPH_Y - this->height, BAR_WIDTH, this->height);
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
  u8g2.begin();  

  //config init
  items[0] = new graphItem("RPM", PID_RPM);
  items[0]->maxValue = 9000;
  //items[1] = new graphItem("KMH", PID_SPEED, 0, 210);
  items[1] = new barItem("Throttle", PID_THROTTLE, 0, 100);
  //items[2] = new graphItem("Oil Temp", PID_ENGINE_OIL_TEMP);
  
#ifdef OBD_ON
  //obd init 
  obd.begin();
  while (!obd.init()); 
#endif
}

void loop(void) {

  //display if we arent going to overshoot the target fps
  unsigned long now = millis();
  if (now - lastFrame > TARGET_FPS) {

#ifndef OBD_ON
    Serial.println(now - lastFrame);
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
    
    u8g2.clearBuffer();
    u8g2.drawHLine(0, GRAPH_Y, DISPLAY_X + BAR_WIDTH); // line between bottom section and graph
    for (int i = 0; i < ITEM_COUNT; i++) {
      if (items[i]->active) 
        items[i]->draw();
    }
    u8g2.sendBuffer();

    lastFrame = now;
  }
}

//acceleration map
//composite item
//allow switching between graphs
//some sort of menu