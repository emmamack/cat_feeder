//      ******************************************************************
//      *                                                                *
//      *         Simple example for controlling a stepper motor         *
//      *                                                                *
//      *            S. Reifel & Co.                6/24/2018            *
//      *                                                                *
//      ******************************************************************


// This is the simplest example of how to run a stepper motor.  
//
// Documentation for this library can be found at:
//    https://github.com/Stan-Reifel/SpeedyStepper
//
//
// This library requires that your stepper motor be connected to the Arduino 
// using drive electronics that has a "Step and Direction" interface.  
// Examples of these are:
//
//    Pololu's DRV8825 Stepper Motor Driver Carrier:
//        https://www.pololu.com/product/2133
//
//    Pololu's A4988 Stepper Motor Driver Carrier:
//        https://www.pololu.com/product/2980
//
//    Sparkfun's Big Easy Driver:
//        https://www.sparkfun.com/products/12859
//
//    GeckoDrive G203V industrial controller:
//        https://www.geckodrive.com/g203v.html
//
// For all driver boards, it is VERY important that you set the motor 
// current before running the example.  This is typically done by adjusting
// a potentiometer on the board.  Read the driver board's documentation to 
// learn how.

// ***********************************************************************


#include <SpeedyStepper.h>


//
// pin assignments
//
const int LED_PIN = 13;
const int MOTOR_STEP_PIN = 6;
const int MOTOR_DIRECTION_PIN = 5;
const int M0_PIN = 9;
const int M1_PIN = 8;
const int M2_PIN = 7;
const int EN_PIN = 10;
#define HOME_PIN A7

//
// create the stepper motor object
//
SpeedyStepper stepper;


bool isAligned() {
  int home_data = analogRead(HOME_PIN);
  return (home_data < 500);
}

void setup() 
{
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW); 
  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  pinMode(M2_PIN, OUTPUT);
  digitalWrite(M0_PIN, HIGH);
  digitalWrite(M1_PIN, HIGH);
  digitalWrite(M2_PIN, LOW);
  
  Serial.begin(9600);

  stepper.connectToPins(MOTOR_STEP_PIN, MOTOR_DIRECTION_PIN);
  stepper.setSpeedInStepsPerSecond(100*32);
  stepper.setAccelerationInStepsPerSecondPerSecond(100*64);

//  while (!isAligned()) {
//    stepper.moveRelativeInSteps(200*8 / 12);
//  }
}

void loop() 
{
  digitalWrite(EN_PIN, LOW); 
  stepper.moveRelativeInSteps(200*8 / 12 * 63);
  digitalWrite(EN_PIN, HIGH);
  delay(1000);
}
