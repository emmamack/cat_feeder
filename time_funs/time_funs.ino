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
  display.setCursor(x, y);
  display.setTextSize(textSize);
  display.setTextColor(WHITE); //TODO: don't hard code this
  
//  display.fillRect(x, y, 25, 25, WHITE);
//  display.setTextColor(BLACK);

  uint8_t hour_rep = time.hour % 12;
  if (time.hour == 12|| time.hour == 0) hour_rep = 12;
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

void saveFeedTime(Time time, int mealNum) {
  EEPROM.write(mealNum*3,   time.hour);
  EEPROM.write(mealNum*3+1, time.minute);
  EEPROM.write(mealNum*3+2, time.second);
}

Time getFeedTime(int mealNum) {
  Time time;
  time.hour   = EEPROM.read(mealNum*3);
  time.minute = EEPROM.read(mealNum*3+1);
  time.second = EEPROM.read(mealNum*3+2);
  return time;
}

void updateButtons() {
  int A1Val = analogRead(A1);
  int A2Val = analogRead(A2);
  int A3Val = analogRead(A3);

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
  Serial.print("Title: "); Serial.println(title);
  Time setTime = {12, 0, 0};
  char timeSections [3] = {'h','m','t'};
  uint8_t timeSectionInd = 0;

  while(1) {
    updateButtons();
    if (wasPressedA1) {
      Serial.println("A1 was pressed!");
      setTime = incrementTime(setTime, false, timeSections[timeSectionInd]);
    }
    
    if (wasPressedA2) {
      Serial.println("A2 was pressed!");
      if (timeSectionInd >= 1) {
        Serial.println("Back to main");
        return setTime;
      }
      timeSectionInd += 1;
    }
    
    if (wasPressedA3) {
      Serial.println("A3 was pressed!");
      setTime = incrementTime(setTime, true, timeSections[timeSectionInd]);
    }
    
    display.clearDisplay();
    displayPrintCenter(title, 5, WHITE);
    displayTime(setTime, 25, 25, 2);
    display.display();
    delay(5);
  }
}

//void userChooseOptions(char* op1, char* op2) {
//  Serial.print("op1: "); Serial.println(op1);
//  Serial.print("op2: "); Serial.println(op2);
//  uint8_t border = 4;
//  bool isHighlighted1 = true;
//  
//  while(1) {
//    display.clearDisplay();
//    updateButtons();
//
//    if (wasPressedA1 || wasPressedA3) isHighlighted1^true;
//
//    if (isHighlighted1) {
//      Serial.println("1 is highlighted");
//      display.fillRect(border, border, SCREEN_WIDTH - 2*border, SCREEN_HEIGHT/2 - border, WHITE);
//      displayPrintCenter(op1, SCREEN_HEIGHT/4, BLACK);
//      display.drawRect(border, (SCREEN_HEIGHT+border)/2, SCREEN_WIDTH - 2*border, SCREEN_HEIGHT/2 - border, WHITE);
//      displayPrintCenter(op2, SCREEN_HEIGHT*3/4, WHITE);
//    } else {
//      Serial.println("2 is highlighted");
//    }
//    
//    display.display();
//    delay(30);
//  }
//}

void setup () {
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);  
  
  Serial.begin(9600);
  #ifndef ESP8266
  while (!Serial); // wait for serial port to connect. Needed for native USB
  #endif
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, need to get time from the user");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
}


void loop () {
  Time currTime = getCurrentTime();
  display.clearDisplay();
  
  displayTime(currTime, 20, 10, 2);
  
  Time breakfastTime = getFeedTime(0);
  display.setCursor(25, 30);
  char bTime[] = "Breakfast time:";
  displayPrintCenter(bTime, 30, WHITE);
  displayTime(breakfastTime, 35, 45, 1);
  
  display.display();
  updateButtons();

  if (wasPressedA2) {
    Serial.println("Going to choose options");
    char op1[] = "Choose meal times";
    char op2[] = "Set current time";
//    userChooseOptions(op1, op2);
  }
  
  if (wasPressedA3) { 
    Serial.println("Going to set time mode");
    char title[] = "Set current time";
    Time setTime = userSetTime(title);
    setCurrentTime(setTime);
  }

  if (wasPressedA1) {
    Serial.println("Setting breakfast time");
    char title[] = "Set breakfast time";
    Time breakfastTime = userSetTime(title);
    saveFeedTime(breakfastTime, 0);
  }
  
  delay(30);
}
