//configuration
#define BAR_WIDTH 4
#define DISPLAY_X 128 - BAR_WIDTH
#define GRAPH_Y 56 //64
#define DISPLAY_Y 64 //64
#define TARGET_FPS 120 //hz
#define SERIAL_ON
#define ITEM_COUNT 2
//#define OBD_ON

#include <U8g2lib.h>
#include <OBD2UART.h>

//hardware
COBD obd;
U8G2_SH1106_128X64_NONAME_1_4W_HW_SPI u8g2(U8G2_R0, 10, 9, 8);

//item that is currently displayed
int currentItem = 0;
int subItem = 0;

int obdGetValue(int type) {
  
  int value;
  if (obd.readPID(type, value)) {
    return value;
  }

  return 0; //oh no
}

int scaleValueToScreenY(float val, float min, float max) {

  // val = 5000
  // min = 0    -> 0
  // max = 9000 -> 64

  return GRAPH_Y - ((val / max) * GRAPH_Y);
}

class item {
public:

  bool active;

  char* name;
  int pid;
  float minValue;
  float maxValue;

  virtual void addValue(float val);
  virtual void draw();
  virtual item(char* name, int pid, int minValue, int maxValue) {
    active = true;

    this->name = name;
    this->pid = pid;
    this->minValue = minValue;
    this->maxValue = maxValue;
  }
};

class graphItem : public item {
public:
  
  float average;
  float currentValue;
  
  uint64_t  count;

  int graph[DISPLAY_X];
  
  graphItem(char* name, int pid, int minValue, int maxValue) : item(name, pid, minValue, maxValue) {

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
    }
    
    //add new value
    this->currentValue = val;
    this->graph[DISPLAY_X - 1] = scaleValueToScreenY(val, this->minValue, this->maxValue);
  }

  void draw() {    
    for (int x = 0; x < DISPLAY_X; x++) {
      u8g2.drawPixel(x, this->graph[x]);  
    }

    u8g2.setCursor(0, DISPLAY_Y);
    if (subItem == 0) {
      u8g2.print(u8x8_u16toa(this->currentValue, 4));
      u8g2.print(" ");
    } else if (subItem == 1) {
      u8g2.print(u8x8_u16toa(this->average, 4));
      u8g2.print(" ");
    } else if (subItem == 2) {
      u8g2.print(millis());
      u8g2.print(" ");
    }
  } 
};

class barItem : public item {
public:
  int height;

  barItem(char* name, int pid, int minValue, int maxValue) : item(name, pid, minValue, maxValue){

    this->height = 0;
  }

  void addValue(float val){

    this->height = scaleValueToScreenY(val, this->minValue, this->maxValue);
  }

  void draw(){

    u8g2.drawBox(DISPLAY_X, GRAPH_Y - this->height, BAR_WIDTH, this->height);
  }
};

//holds item definitions
item* items[ITEM_COUNT] = {};

//times
unsigned long lastFrame = 0;
unsigned long lastSubSwap = 0;

void setup(void) {

#ifdef SERIAL_ON
  //connect to Serial monitor over USB
  Serial.begin(9600);
  Serial.println("START");
#endif

  //display init
  u8g2.begin();
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x10_tn);
    u8g2.setCursor(0, 0);
    u8g2.print("INITIALISING");
  } while ( u8g2.nextPage() );

  //config init
  items[0] = new graphItem("RPM", PID_RPM, 0, 9000);
  //items[1] = new graphItem("KMH", PID_SPEED, 0, 210);
  items[1] = new barItem("Load", PID_ENGINE_LOAD, 0, 100);
  
#ifdef OBD_ON
  //obd init 
  obd.begin();
  while (!obd.init()); 
#endif
}

void loop(void) {

  //display if we arent going to overshoot the target fps
  unsigned long now = millis();
  if (now - lastFrame > (1000 / TARGET_FPS)) {

#ifdef SERIAL_ON
    Serial.println(now - lastFrame);
#endif

    //collect values from obd
    for (int i = 0; i < ITEM_COUNT; i++) {
#ifdef OBD_ON
      int val = obdGetValue(it->PID);
#else
      int val = random(items[i]->maxValue);  
#endif
      items[i]->addValue(val);
    }
    
    u8g2.firstPage();
    do {
      for (int i = 0; i < ITEM_COUNT; i++) {
        if (items[i]->active) 
          items[i]->draw();
      }
    } while ( u8g2.nextPage() );

    //swap through sub displays
    lastSubSwap += (now - lastFrame);
    if (lastSubSwap > 5000) {
      subItem++;
      if (subItem > 2) {
        subItem = 0;
      }
      lastSubSwap = 0;
    }

    lastFrame = now;
  }
}

