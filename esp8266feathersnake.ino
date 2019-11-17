#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include "Adafruit_miniTFTWing.h"
#include <Queue.h>

Adafruit_miniTFTWing ss;
#define TFT_RST    -1    // we use the seesaw for resetting to save a pin

#ifdef ESP8266
   #define TFT_CS   2
   #define TFT_DC   16
#endif
#ifdef ESP32
   #define TFT_CS   14
   #define TFT_DC   32
#endif
#ifdef TEENSYDUINO
   #define TFT_CS   8
   #define TFT_DC   3
#endif
#ifdef ARDUINO_STM32_FEATHER
   #define TFT_CS   PC5
   #define TFT_DC   PC7
#endif
#ifdef ARDUINO_NRF52832_FEATHER /* BSP 0.6.5 and higher! */
   #define TFT_CS   27
   #define TFT_DC   30
#endif

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

enum Direction { up, down, left, right };

#define GRID_X 20
#define GRID_Y 10

struct coord {
  uint8_t x;
  uint8_t y;
};

struct move_response {
  struct coord *to_remove;
  struct coord *to_draw;
};

class Snake {
  struct body {
    struct coord * xy;
    struct body *next;
    struct body *prev;
  };
  Direction dir = right;
  struct body *head;
  struct body *tail;
  bool snake_map[GRID_X][GRID_Y] = {{false}};
  uint16_t len;
  
  public:
  Snake(){
    head = new body;
    head->xy = new coord;
    head->xy->x = 1;
    head->xy->y = 0;

    tail = new body;
    tail->xy = new coord;
    tail->xy->x = 0;
    tail->xy->y = 0;

    head->next = NULL;
    head->prev = tail;
    
    tail->next = head;
    tail->prev = NULL;

    snake_map[0][0] = true;
    snake_map[1][0] = true;

    len = 2;
  }

  void changeDir(Direction dir){
    //do not allow moving into opposite dir
    if(this->dir == right && dir == left){
      return;
    }
    if(this->dir == left && dir == right){
      return;
    }
    if(this->dir == up && dir == down){
      return;
    }
    if(this->dir == down && dir == up){
      return;
    }
    this->dir = dir;
  }

  coord * freeSpot(){
    uint8_t x = random(0, GRID_X - 1);
    uint8_t y = random(0, GRID_Y - 1);
    bool visited[GRID_X][GRID_Y] = {{false}};
    DataQueue<int> xQueue(20);
    DataQueue<int> yQueue(20);
    xQueue.enqueue(x);
    yQueue.enqueue(y);
    while(!xQueue.isEmpty()){
      x = xQueue.dequeue();
      y = yQueue.dequeue();
      visited[x][y] = true;
      if(!snake_map[x][y]){
        break;
      }
      if((x + 1) <= GRID_X && !visited[x+1][y]){
        xQueue.enqueue(x+1);
        yQueue.enqueue(y);
      }
      if((x - 1) >= 0 && !visited[x-1][y]){
        xQueue.enqueue(x-1);
        yQueue.enqueue(y);
      }
      if((y + 1) <= GRID_Y && !visited[x][y+1]){
        xQueue.enqueue(x);
        yQueue.enqueue(y+1);
      }
      if((y - 1) >= 0 && !visited[x][y-1]){
        xQueue.enqueue(x);
        yQueue.enqueue(y-1);
      }
    }
    if(snake_map[x][y]){
      return NULL;
    }
    coord *ret = new coord;
    ret->x = x;
    ret->y = y;
    return ret;
  }

  uint16_t getLength(){
      return len;
    }

  move_response * tick(uint8_t x, uint8_t y){
    struct coord *move_to = new coord;
    move_to->x = head->xy->x;
    move_to->y = head->xy->y;
    switch(dir){
      case right:
        move_to->x++;
        if(move_to->x >= GRID_X){
          return NULL;
        }
        break;
      case left:
      if(move_to->x == 0){
          return NULL;
        }
        move_to->x--;
        break;
      case down:
        move_to->y++;
        if(move_to->y >= GRID_Y){
          return NULL;
        }
        break;
      case up:
        if(move_to->y == 0){
          return NULL;
        }
        move_to->y--;
        break;
    }
    if(snake_map[move_to->x][move_to->y]){
      return NULL;
    }

    bool feed = false;
    if(x == move_to->x && y == move_to->y){
      feed = true;
      len++;
    }

    snake_map[move_to->x][move_to->y] = true;
    struct move_response *response = new move_response;
    response->to_draw = move_to;
    if(!feed){
      response->to_remove = new coord;
      response->to_remove->x = tail->xy->x;
      response->to_remove->y = tail->xy->y;
    } else {
      response->to_remove = NULL;
    }
    struct body *new_head = new body;
    //Add new head
    new_head->xy = move_to;
    new_head->prev = head;
    new_head->next = NULL;
    head->next = new_head;
    head = new_head;
    
    if(!feed){
      //Remove entry from map
      snake_map[tail->xy->x][tail->xy->y] = false;
      struct body *new_tail = tail->next;
      new_tail->prev = NULL;
      tail->next = NULL;
      delete tail->xy;
      tail->xy = NULL;
      delete tail;
      tail = new_tail;
    }
    
    return response;
  }
};

float p = 3.1415926;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  Serial.print("Hello! ST77xx TFT Test");
  
  if (!ss.begin()) {
    Serial.println("seesaw init error!");
    while(1);
  }
  else Serial.println("seesaw started");

  ss.tftReset();
  ss.setBacklight(0x0); //set the backlight fully on

  // Use this initializer (uncomment) if you're using a 0.96" 180x60 TFT
  tft.initR(INITR_MINI160x80);   // initialize a ST7735S chip, mini display

  tft.setRotation(3);
    
  Serial.println("Initialized");

  uint16_t time = millis();
  tft.fillScreen(ST77XX_BLACK);
  time = millis() - time;
  
  /*
  tft.fillScreen(ST77XX_BLACK);
  for(int x = 0; x < 20; x++){
    for(int y = 0; y < 10; y++){
      drawSquare(x,y);
      delay(100);
    }
  }
  */
}

void drawSquare(uint16_t x, uint16_t y, uint16_t color){
  x = x * 8;
  y = y * 8;
  //tft.drawRect(x, y , 8, 8, ST77XX_BLACK);
  tft.fillRect(x + 1, y + 1 , 6, 6, color);
}

void clearSquare(uint16_t x, uint16_t y){
  Serial.print("Clear ");
  Serial.print(x);
  Serial.print(", ");
  Serial.println(y);
  x = x * 8;
  y = y * 8;
  tft.fillRect(x, y , 8, 8, ST77XX_BLACK);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(10);
  startScreen();
  snake();
}

void startScreen(){
  uint32_t buttons;
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_BLUE);
  tft.setTextWrap(false);
  tft.setCursor(0, 0);
  tft.setTextSize(3);
  tft.println("  Snake");
  tft.println("");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  delay(500);
  tft.println("Press any button to start");
  bool play = false;
  while(!play){
    delay(10);
    buttons = ss.readButtons();
    
    if (! (buttons & TFTWING_BUTTON_LEFT)) {
      play = true;
    }
 
    if (! (buttons & TFTWING_BUTTON_RIGHT)) {
      play = true;
    }
  
    if (! (buttons & TFTWING_BUTTON_DOWN)) {
      play = true;
    }
  
    if (! (buttons & TFTWING_BUTTON_UP)) {
      play = true;
    }

    if (! (buttons & TFTWING_BUTTON_A)) {
      play = true;
    }

    if (! (buttons & TFTWING_BUTTON_B)) {
      play = true;
    }

    if (! (buttons & TFTWING_BUTTON_SELECT)) {
      play = true;
    }
  }
}

void snake(){
  uint32_t buttons;
  uint8_t speed = 150;
  uint8_t speed_count = 0;
  tft.fillScreen(ST77XX_BLACK);
  Snake *snake = new Snake();
  drawSquare(0,0, ST77XX_WHITE);
  drawSquare(1,0, ST77XX_WHITE);
  bool alive = true;
  unsigned long last_draw = millis();
  coord *food = food = snake->freeSpot();
  drawSquare(food->x, food->y, ST77XX_GREEN);
  while(alive){
    delay(10);
    buttons = ss.readButtons();
    if (! (buttons & TFTWING_BUTTON_LEFT)) {
      snake->changeDir(left);
    }
 
    if (! (buttons & TFTWING_BUTTON_RIGHT)) {
      snake->changeDir(right);
    }
  
    if (! (buttons & TFTWING_BUTTON_DOWN)) {
      snake->changeDir(down);
    }
  
    if (! (buttons & TFTWING_BUTTON_UP)) {
      snake->changeDir(up);
    }
    if(millis() - last_draw >= speed){
      last_draw = millis();
      speed_count++;
      if(speed_count == 25 && speed > 20){
        speed_count = 0;
        speed--;
      }
      move_response *response = snake->tick(food->x, food->y);
      if(response == NULL){
        alive = false;
        uint16_t score = snake->getLength() - 2;
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextColor(ST77XX_RED);
        tft.setTextWrap(false);
        tft.setCursor(0, 0);
        tft.setTextSize(3);
        tft.println("Game Over");
        tft.setTextSize(2);
        tft.setTextColor(ST77XX_WHITE);
        tft.print("Score ");
        tft.println(score);
        tft.setTextColor(ST77XX_WHITE);
        delay(1500);
        //TODO Cleanup
      } else {
        drawSquare(response->to_draw->x, response->to_draw->y, ST77XX_WHITE);
        if(response->to_remove != NULL){
          clearSquare(response->to_remove->x, response->to_remove->y);
        } else {
          delete food;
          food = snake->freeSpot();
          if(food != NULL){
            drawSquare(food->x, food->y, ST77XX_GREEN);
          }
        }
      }
    }
  }
}
