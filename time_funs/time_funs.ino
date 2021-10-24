#include "RTClib.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1 
#define SCREEN_ADDRESS 0x3C
#define WHITE SSD1306_WHITE
#define BLACK SSD1306_BLACK
#define BORDER 4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;
bool isDownA1 = false; bool wasPressedA1 = false;
bool isDownA2 = false; bool wasPressedA2 = false;
bool isDownA3 = false; bool wasPressedA3 = false;

struct Time {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

Time nextMealTime = {255, 255, 255};

Time getCurrentTime() {
  DateTime now = rtc.now();
  Time currTime = { now.hour(), now.minute(), now.second() };
  return currTime;
}

void displayPrintCenter(char* string, uint8_t cursorY, uint16_t color) {
  int16_t  x1, y1;
  uint16_t w, h;
  display.getTextBounds(string, 0, 0, &x1, &y1, &w, &h);
  uint8_t offset = w/2;
  int8_t cursorX = SCREEN_WIDTH/2 - offset;
  display.setCursor(cursorX, cursorY);
  display.setTextColor(color);
  display.print(string);
  
}

void displayTime(Time time, uint8_t x, uint8_t y, uint8_t textSize) {
  display.setTextSize(textSize);
  display.setTextColor(WHITE); //TODO: don't hard code this
  
//  display.fillRect(x, y, 25, 25, WHITE);
//  display.setTextColor(BLACK);

  int16_t  x1, y1;
  uint16_t w, h;
  if (time.hour % 12 > 9 || time.hour == 12 || time.hour == 0) {
    display.getTextBounds("AAAAAAAA", 0, 0, &x1, &y1, &w, &h);
  } else {
    display.getTextBounds("AAAAAAA", 0, 0, &x1, &y1, &w, &h);
  }
  uint8_t offset = w/2;
  int8_t cursorX = SCREEN_WIDTH/2 - offset;
  display.setCursor(cursorX, y);

  uint8_t hour_rep = time.hour % 12;
  if (time.hour == 12 || time.hour == 0) hour_rep = 12;
  display.print(hour_rep); 
//  display.setTextColor(BLACK);
  display.print(":");
  if (time.minute < 10) display.print(0);
  display.print(time.minute); display.print(" ");
  
  if (time.hour > 11) {
    display.print("PM");
  } else {
    display.print("AM");
  }
  display.setTextSize(1); // reset after
}

void setCurrentTime(Time time) {
  rtc.adjust(DateTime(2000, 1, 1, time.hour, time.minute, time.second));
}

void saveFeedTime(Time time, uint8_t mealNum) {
  EEPROM.write(mealNum*3,   time.hour);
  EEPROM.write(mealNum*3+1, time.minute);
  EEPROM.write(mealNum*3+2, time.second);
}

Time getFeedTime(uint8_t mealNum) {
  Time time;
  time.hour   = EEPROM.read(mealNum*3);
  time.minute = EEPROM.read(mealNum*3+1);
  time.second = EEPROM.read(mealNum*3+2);
  return time;
}

uint16_t getMinutes(Time time) {
  return 60*time.hour + time.minute;
}

void getNextMealTime() {
  uint16_t meal1Mins = getMinutes(getFeedTime(0));
  uint16_t meal2Mins = getMinutes(getFeedTime(1));
  uint16_t currMins  = getMinutes(getCurrentTime());

  if (meal1Mins <= currMins) meal1Mins += 1440;
  if (meal2Mins <= currMins) meal2Mins += 1440;

  if (((meal1Mins - currMins) < (meal2Mins - currMins)) || ((meal2Mins - currMins) == 1440)){
    nextMealTime = getFeedTime(0);
  } 
  if (((meal2Mins - currMins) < (meal1Mins - currMins)) || ((meal1Mins - currMins) == 1440)) {
    nextMealTime = getFeedTime(1);
  }
}

void displayHomeScreen(Time nextMealTime) {
  display.clearDisplay();
  
  Time currTime = getCurrentTime();
  displayTime(currTime, 20, 10, 2);
  
  displayPrintCenter("Next meal time:", 35, WHITE);
  displayTime(nextMealTime, 35, 47, 1);
  
  display.display();
}

void updateButtons() {
  uint16_t A1Val = analogRead(A1);
  uint16_t A2Val = analogRead(A2);
  uint16_t A3Val = analogRead(A3);

  if (wasPressedA1) wasPressedA1 = false;
  if (A1Val < 500) isDownA1 = true;
  if (A1Val > 500 && isDownA1) {
    isDownA1 = false;
    wasPressedA1 = true;
  }

  if (wasPressedA2) wasPressedA2 = false;
  if (A2Val < 500) isDownA2 = true;
  if (A2Val > 500 && isDownA2) {
    isDownA2 = false;
    wasPressedA2 = true;
  }

  if (wasPressedA3) wasPressedA3 = false;
  if (A3Val < 500) isDownA3 = true;
  if (A3Val > 500 && isDownA3) {
    isDownA3 = false;
    wasPressedA3 = true;
  }
}

Time incrementTime(Time time, bool up, char timeSection) {
  if (timeSection == 'h' && up) {
    time.hour = time.hour + 1;
    if (time.hour == 24) time.hour = 0;
  }
  
  if (timeSection == 'h' && !up) {
    time.hour = time.hour - 1;
    if (time.hour == 255) time.hour = 23;
  }

  if (timeSection == 'm' && up) {
    time.minute += 1;
    if (time.minute == 60) time.minute = 0;
  }
  
  if (timeSection == 'm' && !up) {
    time.minute -= 1;
    if (time.minute == 255) time.minute = 59;
  }
  
  return time;
}

Time userSetTime(char* title) {
  Time setTime = {12, 0, 0};
  char timeSections [2] = {'h','m'};
  uint8_t timeSectionInd = 0;

  while(1) {
    updateButtons();
    if (wasPressedA1) {
      setTime = incrementTime(setTime, false, timeSections[timeSectionInd]);
    }
    
    if (wasPressedA2) {
      if (timeSectionInd >= 1) {
        return setTime;
      }
      timeSectionInd += 1;
    }
    
    if (wasPressedA3) {
      setTime = incrementTime(setTime, true, timeSections[timeSectionInd]);
    }
    
    display.clearDisplay();
    displayPrintCenter(title, 5, WHITE);
    displayTime(setTime, 25, 25, 2);
    display.display();
    delay(5);
  }
}

bool userChooseOptions(char* op1, char* op2) {
  bool isHighlighted1 = true; //TODO: this is a little hacky
  
  while(1) {
    display.clearDisplay();
    updateButtons();

    if (wasPressedA1 || wasPressedA3) isHighlighted1^=true;
    
    if (isHighlighted1) {
      display.fillRect(BORDER, BORDER, SCREEN_WIDTH - 2*BORDER, 
                       SCREEN_HEIGHT/2 - BORDER, WHITE);
      displayPrintCenter(op1, SCREEN_HEIGHT/4, BLACK);
      display.drawRect(BORDER, (SCREEN_HEIGHT+BORDER)/2, SCREEN_WIDTH - 2*BORDER, 
                       SCREEN_HEIGHT/2 - BORDER, WHITE);
      displayPrintCenter(op2, SCREEN_HEIGHT*3/4, WHITE);
      
    } else {
      display.drawRect(BORDER, BORDER, SCREEN_WIDTH - 2*BORDER, 
                       SCREEN_HEIGHT/2 - BORDER, WHITE);
      displayPrintCenter(op1, SCREEN_HEIGHT/4, WHITE);
      display.fillRect(BORDER, (SCREEN_HEIGHT+BORDER)/2, SCREEN_WIDTH - 2*BORDER, 
                       SCREEN_HEIGHT/2 - BORDER, WHITE);
      displayPrintCenter(op2, SCREEN_HEIGHT*3/4, BLACK);
    }

    if (wasPressedA2) {
      return isHighlighted1;
    }
    
    display.display();
    delay(30);
  }
}

void printTime(Time time) {
  Serial.print(time.hour);
  Serial.print(":");
  Serial.println(time.minute);
}

void checkIfFeedTime() {
  if (getMinutes(nextMealTime) == getMinutes(getCurrentTime())) {
    Serial.println("TIME TO EAT!");
    // rotateTray();
    delay(1001); //TODO: don't need this once motor rotation exists
  }
}

void setup() {
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);  
  
  Serial.begin(9600);
  #ifndef ESP8266
  while (!Serial); // wait for serial port to connect. Needed for native USB
  #endif
  if (! rtc.begin()) {
    Serial.println("RTC f");
    Serial.flush();
    abort();
  }
  if (rtc.lostPower()) {
    Serial.println("RTC p");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Al F"));
    for(;;); // Don't proceed, loop forever
  }
}

void loop() {
  checkIfFeedTime();
  getNextMealTime();
  Serial.print("next "); printTime(nextMealTime);
  Serial.print("1 "); printTime(getFeedTime(0));
  Serial.print("2 "); printTime(getFeedTime(1));
  displayHomeScreen(nextMealTime);
  updateButtons();

  if (wasPressedA1 || wasPressedA2 || wasPressedA3) {
    bool menuSelection = userChooseOptions("Choose meal times", "Set current time");
    
    if (menuSelection) {
      bool mealSelection = userChooseOptions("Meal 1", "Meal 2");
      
      if (mealSelection) {
        Time meal1Time = userSetTime("Set meal 1 time");
        saveFeedTime(meal1Time, 0);

      } else {
        Time meal2Time = userSetTime("Set meal 2 time");
        saveFeedTime(meal2Time, 1);
      }
      
    } else {
      Time setTime = userSetTime("Set current time");
      setCurrentTime(setTime);
    }
  }
  
  delay(30);
}
