//configuration
#define DISPLAY_X 128
#define GRAPH_Y 56 //64
#define DISPLAY_Y 64 //64
#define TARGET_FPS 120 //hz
#define SERIAL_ON
//#define OBD_ON

#include <U8g2lib.h>
#include <OBD2UART.h>

COBD obd;

int obdGetValue(int type) {
  
  int value;
  if (obd.readPID(type, value)) {
    return value;
  }

  return 0; //oh no
}

//hardware
U8G2_SH1106_128X64_NONAME_1_4W_HW_SPI u8g2(U8G2_R0, 10, 9, 8);

int scaleValueToScreenY(float val, float min, float max) {

  // val = 5000
  // min = 0    -> 0
  // max = 9000 -> 64

  return GRAPH_Y - ((val / max) * GRAPH_Y);
}

class item {
public:
  char* name;
  int pid;
  float average;
  float currentValue;
  uint64_t  count;

  int graph[DISPLAY_X];
  
  item() {}
  item(char* name, int pid) {

    this->name = name;
    this->pid = pid;

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
    this->graph[DISPLAY_X - 1] = scaleValueToScreenY(val, 0, 9000);
    this->currentValue = val;
  }
  
};

//holds item definitions
item items[2] = {};

//item that is currently displayed
int currentItem = 0;
int subItem = 0;

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
  items[0] = item("RPM", PID_RPM);
  items[1] = item("MPH", PID_SPEED);
  
#ifdef OBD_ON
  //obd init 
  obd.begin();
  while (!obd.init()); 
#endif
}

void draw() {    
  u8g2.firstPage();
  do {
    for (int x = 0; x < DISPLAY_X; x++) {
      u8g2.drawPixel(x, items[currentItem].graph[x]);  
    }

    u8g2.setCursor(0, DISPLAY_Y);
    if (subItem == 0) {
      u8g2.print(u8x8_u16toa(items[currentItem].currentValue, 4));
      u8g2.print(" ");
    } else if (subItem == 1) {
      u8g2.print(u8x8_u16toa(items[currentItem].average, 4));
      u8g2.print(" ");
    } else if (subItem == 2) {
      u8g2.print(millis());
      u8g2.print(" ");
    }
    
  } while ( u8g2.nextPage() );
} 

void loop(void) {

//collect values from obd
  for (int i = 0; i < 2; i++) {
#ifdef OBD_ON
    int val = obdGetValue(it->PID);
#else
    int val = random(9000);  
#endif
    items[i].addValue(val);
  }

  //display if we arent going to overshoot the target fps
  unsigned long now = millis();
  if (now - lastFrame > (1000 / TARGET_FPS)) {

#ifdef SERIAL_ON
    Serial.println(now - lastFrame);
#endif
    
    draw();

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

