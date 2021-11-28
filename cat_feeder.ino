#include "RTClib.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FlexyStepper.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1 
#define SCREEN_ADDRESS 0x3C
#define WHITE SSD1306_WHITE
#define BLACK SSD1306_BLACK
#define BORDER 4

#define HOME_PIN A7
#define UP_PIN A3
#define MID_PIN A2
#define DOWN_PIN A1
#define MOTOR_STEP_PIN 6
#define MOTOR_DIRECTION_PIN 5
#define M0_PIN 9
#define M1_PIN 8
#define M2_PIN 7
#define EN_PIN 10

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;
FlexyStepper stepper;

bool isDownUP = false;   bool wasPressedUP = false;
bool isDownMID = false;  bool wasPressedMID = false;
bool isDownDOWN = false; bool wasPressedDOWN = false;
int pressCounter = 0;

struct Time {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

Time nextMealTime = {255, 255, 255};
bool blinkOn = false; uint8_t blinkCounter = 0;

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

void handleBlinks(uint8_t numCycles) {
  if (blinkCounter >= numCycles) {
    blinkOn ^= true;
    blinkCounter = 0;
  }
  blinkCounter += 1;
}

void displayTime(Time time, uint8_t cursorY, uint8_t textSize, uint8_t highlight, bool blinking) {
  
  display.setTextColor(WHITE);
  
  display.setTextSize(textSize);
  int16_t  x1, y1;
  uint16_t wUnit, hUnit;
  display.getTextBounds("0", 0, 0, &x1, &y1, &wUnit, &hUnit);
  uint8_t wHighlight = wUnit*2 + 2;
  int8_t cursorX = SCREEN_WIDTH/2 - wUnit/2*8;
  if ((time.hour % 12 < 10) && (time.hour != 0) && (time.hour != 12)) {
    cursorX = SCREEN_WIDTH/2 - wUnit/2*7;
    wHighlight = wUnit + 2;
  }
  display.setCursor(cursorX, cursorY);


  if (highlight == 1) {
    display.setTextColor(BLACK);
    display.fillRect(cursorX - 2, cursorY - 2, wHighlight, hUnit + 2, WHITE);
  }
  uint8_t hour_rep = time.hour % 12;
  if (time.hour == 12 || time.hour == 0) hour_rep = 12;
  display.print(hour_rep); 
  if (highlight == 1) display.setTextColor(WHITE);
  if (blinking && !blinkOn) {
    display.print(" ");
  } else {
    display.print(":");
  }

  
  if (highlight == 2) {
    display.fillRect(cursorX + 8 + wHighlight, cursorY - 2, wUnit*2 + 2, hUnit + 2, WHITE);
    display.setTextColor(BLACK);
  }
  if (time.minute < 10) display.print(0);
  display.print(time.minute); display.print(" ");
  if (highlight == 2) display.setTextColor(WHITE);


  if (highlight == 3) {
    cursorX += wUnit*3;
    display.fillRect(cursorX + 8 + wHighlight, cursorY - 2, wUnit*2 + 2, hUnit + 2, WHITE);
    display.setTextColor(BLACK);
  }
  if (time.hour > 11) {
    display.print("PM");
  } else {
    display.print("AM");
  }
  if (highlight == 3) {
      display.setTextColor(WHITE);
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
  handleBlinks(9);
  displayTime(currTime, 10, 2, 0, true);
  
  displayPrintCenter("Next meal time", 35, WHITE);
  displayTime(nextMealTime, 47, 1, 0, false);
  
  display.display();
}

void updateButtons() {
  uint16_t UPVal = analogRead(UP_PIN);
  uint16_t MIDVal = analogRead(MID_PIN);
  uint16_t DOWNVal = analogRead(DOWN_PIN);

  if (wasPressedUP) wasPressedUP = false;
  if (UPVal < 500) {
    isDownUP = true;
    pressCounter += 1;
    if (pressCounter >= 15 && pressCounter % 2 == 0) wasPressedUP = true; // handle long presses
  }
  if (UPVal > 500 && isDownUP) {
    isDownUP = false;
    pressCounter = 0;
    wasPressedUP = true;
  }

  if (wasPressedMID) wasPressedMID = false;
  if (MIDVal < 500) {
    isDownMID = true;
    pressCounter += 1;
    if (pressCounter >= 15 && pressCounter % 2 == 0) wasPressedMID = true;
  }
  if (MIDVal > 500 && isDownMID) {
    isDownMID = false;
    pressCounter = 0;
    wasPressedMID = true;
  }

  if (wasPressedDOWN) wasPressedDOWN = false;
  if (DOWNVal < 500) {
    isDownDOWN = true;
    pressCounter += 1;
    if (pressCounter >= 15 && pressCounter % 2 == 0) wasPressedDOWN = true;
  }
  if (DOWNVal > 500 && isDownDOWN) {
    isDownDOWN = false;
    pressCounter = 0;
    wasPressedDOWN = true;
  }
}

Time incrementTime(Time time, bool up, char timeSection) {
  if (timeSection == 1 && up) {
    time.hour = time.hour + 1;
    if (time.hour == 24) time.hour = 0;
  }
  
  if (timeSection == 1 && !up) {
    time.hour = time.hour - 1;
    if (time.hour == 255) time.hour = 23;
  }

  if (timeSection == 2 && up) {
    time.minute += 1;
    if (time.minute == 60) time.minute = 0;
  }
  
  if (timeSection == 2 && !up) {
    time.minute -= 1;
    if (time.minute == 255) time.minute = 59;
  }

  if (timeSection == 3) {
    time.hour += 12;
    time.hour = time.hour % 24;
  }
  
  return time;
}

Time userSetTime(char* title, Time setTime) {
  uint8_t timeSection = 1;

  while(1) {
    updateButtons();
    if (wasPressedUP) {
      setTime = incrementTime(setTime, false, timeSection);
    }
    
    if (wasPressedMID) {
      if (timeSection >= 3) {
        return setTime;
      }
      timeSection += 1;
    }
    
    if (wasPressedDOWN) {
      setTime = incrementTime(setTime, true, timeSection);
    }
    
    display.clearDisplay();
    displayPrintCenter(title, 10, WHITE);
    displayTime(setTime, 30, 2, timeSection, false);
    display.display();
    delay(5);
  }
}

bool userChooseOptions(char* op1, char* op2) {
  bool isHighlighted1 = true;
  
  while(1) {
    display.clearDisplay();
    updateButtons();

    if (wasPressedUP || wasPressedDOWN) isHighlighted1^=true;
    
    if (isHighlighted1) {
      display.fillRect(BORDER, BORDER, SCREEN_WIDTH - 2*BORDER, 
                       SCREEN_HEIGHT/2 - BORDER, WHITE);
      displayPrintCenter(op1, SCREEN_HEIGHT/4, BLACK);
      display.drawRect(BORDER, (SCREEN_HEIGHT+BORDER)/2, SCREEN_WIDTH - 2*BORDER, 
                       SCREEN_HEIGHT/2 - BORDER, WHITE);
      displayPrintCenter(op2, SCREEN_HEIGHT*3/4 - 4, WHITE);
      
    } else {
      display.drawRect(BORDER, BORDER, SCREEN_WIDTH - 2*BORDER, 
                       SCREEN_HEIGHT/2 - BORDER, WHITE);
      displayPrintCenter(op1, SCREEN_HEIGHT/4, WHITE);
      display.fillRect(BORDER, (SCREEN_HEIGHT+BORDER)/2, SCREEN_WIDTH - 2*BORDER, 
                       SCREEN_HEIGHT/2 - BORDER, WHITE);
      displayPrintCenter(op2, SCREEN_HEIGHT*3/4, BLACK);
    }

    if (wasPressedMID) {
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

bool isAligned() {
  int home_data = analogRead(HOME_PIN);
  return (home_data < 500);
}

void enableMotor() {
  digitalWrite(EN_PIN, LOW);
}

void disableMotor() {
  digitalWrite(EN_PIN, HIGH);
}

void homeTray() {
  if (isAligned()) return;
  
  display.clearDisplay();
  displayPrintCenter("Initializing...", SCREEN_HEIGHT/2, WHITE);
  display.display();

  enableMotor();
  stepper.setSpeedInStepsPerSecond(1600);
  stepper.setAccelerationInStepsPerSecondPerSecond(3200);
  stepper.setTargetPositionInSteps(200*8 / 12 * 63 + 10);
  stepper.setCurrentPositionInSteps(0);
  bool stopFlag = false;

  while(!stepper.motionComplete()) {
    stepper.processMovement();
    if (isAligned() && (stopFlag == false)) {
      stepper.setTargetPositionToStop();
      stopFlag = true;
    }
  }
  stepper.moveRelativeInSteps(-240);
  disableMotor();
}

void rotateTray() {
  enableMotor();
  stepper.moveRelativeInSteps(200*8 / 12 * 63 + 10);
  disableMotor();
}

void checkIfFeedTime() {
  if (getMinutes(nextMealTime) == getMinutes(getCurrentTime())) {
    rotateTray();
  }
}

void setup() {
  Serial.begin(9600);
  
  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(MID_PIN, INPUT_PULLUP);
  pinMode(DOWN_PIN, INPUT_PULLUP); 
  pinMode(EN_PIN, OUTPUT);
  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  pinMode(M2_PIN, OUTPUT); 
  digitalWrite(M0_PIN, HIGH);
  digitalWrite(M1_PIN, HIGH);
  digitalWrite(M2_PIN, LOW); 
  disableMotor();

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
  
  display.setTextSize(1);
  stepper.connectToPins(MOTOR_STEP_PIN, MOTOR_DIRECTION_PIN);
  homeTray();
}

void loop() {
  checkIfFeedTime();
  getNextMealTime();
  displayHomeScreen(nextMealTime);
  updateButtons();

  if (wasPressedUP || wasPressedMID || wasPressedDOWN) {
    bool menuSelection = userChooseOptions("Set meal times", "Set current time");
    
    if (menuSelection) {
      bool mealSelection = userChooseOptions("First meal", "Second meal");
      
      if (mealSelection) {
        char title[] = "Set first meal time";
        Time meal1Time = userSetTime(title, getFeedTime(0));
        saveFeedTime(meal1Time, 0);

      } else {
        char title[] = "Set second meal time";
        Time meal2Time = userSetTime(title, getFeedTime(1));
        saveFeedTime(meal2Time, 1);
      }
      
    } else {
      char title[] = "Set current time";
      Time setTime = userSetTime(title, getCurrentTime());
      setCurrentTime(setTime);
    }
  }
  
  delay(30);
}
