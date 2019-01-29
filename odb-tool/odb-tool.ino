//configuration
#define DISPLAY_X 128
#define DISPLAY_Y 56 //64
#define TARGET_FPS 60 //hz
#define SERIAL_ON
//#define OBD_ON

#include <U8g2lib.h>

#ifdef OBD_ON
#include <OBD2UART.h>

COBD obd;

int obdGetValue(int type) {
  return 9000;
}

#endif

//hardware
U8G2_SH1106_128X64_NONAME_1_4W_HW_SPI u8g2(U8G2_R0, 10, 9, 8);

int scaleValueToScreenY(float val, float min, float max) {

  // val = 5000
  // min = 0    -> 0
  // max = 9000 -> 64

  return DISPLAY_Y - ((val / max) * DISPLAY_Y);
}

class item {
public:
  char* name;
  float average;
  float currentValue;
  uint64_t  count;

  int graph[DISPLAY_X];
  
  item() {
    average = 0;
    count = 0;

    for (int i = 0; i < DISPLAY_X; i++) {
      //this->graph[i] = DISPLAY_Y;
      this->graph[i] = DISPLAY_Y * (1 + sin((float(i) / float(DISPLAY_X)) * 2 * M_PI)) / 2;
    }
  }
  
  void draw() {    
    u8g2.firstPage();
    do {
      for (int x = 0; x < DISPLAY_X; x++) {
        u8g2.drawPixel(x, this->graph[x]);  
      }
  
      u8g2.setCursor(0, 64);
      u8g2.print(u8x8_u16toa(currentValue, 4)); //maybe cast to string?
    } while ( u8g2.nextPage() );
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
item items[1] = {};

//item that is currently displayed
int currentItem = 0;

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
    u8g2.drawStr(0, 0, "INITIALISING");
  } while ( u8g2.nextPage() );

  //config init
  items[0] = item();
  
#ifdef OBD_ON
  //obd init
  obd.begin();
  while (!obd.init()); 
#endif
}

unsigned long lastFrame = millis();
void loop(void) {

  //display if we arent going to overshoot the target fps
  unsigned long now = millis();
  if (now - lastFrame > (1000 / TARGET_FPS)) {

#ifdef SERIAL_ON
    Serial.println(now - lastFrame);
#endif

    int val = 0;

#ifdef OBD_ON

    val = obdGetValue(PID_RPM);
    
#endif

    items[currentItem].addValue(random(9000));
    
    items[currentItem].draw();

    lastFrame = now;
  }
}

