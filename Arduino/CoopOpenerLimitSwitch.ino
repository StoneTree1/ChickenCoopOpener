#include <SPI.h>
#include <Wire.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
#include <Stepper.h>
#include <Keypad.h>
#include <EEPROM.h>
#include "LowPower.h"
//#define OLED_RESET 4
//Adafruit_SSD1306 display(OLED_RESET);

#define UP 0
#define DOWN 1
#define IN1  10
#define IN2  11
#define IN3  12
#define IN4  13
int NextStep = 0;
long stepsPerRevolution = 2048;
int revolutionsToOpen = 1;
boolean Direction = true;
unsigned long last_time;
unsigned long currentMillis ;
long time;
char lastKey;
unsigned long last_manual_time;
unsigned long last_door_change_time;
unsigned long last_sensor_check_time;
const byte ROWS = 1;
const byte COLS = 4;

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', '4'}
};

byte colPins[COLS] = {8, 9, 6, 7};byte rowPins[ROWS] = {5};

//const int stepsPerRevolution = 1100;  // change this to fit the number of steps per revolution
//const int stepDistance = 200;
int photocellPin = 0;     // the cell and 10K pulldown are connected to a0
int sensorLimit = 200;
/*-----( Declare objects )-----*/
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
// initialize the stepper library on pins 8 through 11:

int doorOpenSwitch = 2;
int doorClosedSwitch = 3;

void setup() {
pinMode(IN1, OUTPUT); 
pinMode(IN2, OUTPUT); 
pinMode(IN3, OUTPUT); 
pinMode(IN4, OUTPUT); 
pinMode(doorOpenSwitch , INPUT);
pinMode(doorClosedSwitch , INPUT);
   //Serial.begin(4800);
  // put your setup code here, to run once:
last_manual_time = 0;
last_door_change_time = 0;
last_sensor_check_time = 0;
  revolutionsToOpen = EEPROMReadInt(0);
  // Serial.println("steps to open");
  // Serial.println(stepsToOpen);
  int sensorLevel = EEPROMReadInt(3);
  if (sensorLevel < 0) {
    EEPROMWriteInt(3, sensorLimit);
  } else {
    sensorLimit = sensorLevel;
  }
  //this open is a self calibration step
  OpenDoor();
}

void loop() {
  // Serial.println("Main loop");
  // put your main code here, to run repeatedly:
  CheckLightSensor();
  CheckForInput(); 
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  
 //delay(1000);
}

void CheckLightSensor() {
  unsigned long current = micros();
  if(current-last_sensor_check_time >= 1800000){
    last_sensor_check_time = current;
    if(current-last_door_change_time>=1800000){ 
      int photocellReading = analogRead(photocellPin);
       //Serial.println("photo cell");
       //Serial.println(photocellReading);
      bool doorIsOpen;
      EEPROM.get(2, doorIsOpen);
      //now we have to map 0-1023 to 0-255 since thats the range analogWrite uses
      if (photocellReading < sensorLimit) {
        if (doorIsOpen) {
          CloseDoor();
        }
      } else if (photocellReading > sensorLimit + 100) {
        if (!doorIsOpen) {
          OpenDoor();
        }
      }
    }
  }
}

void CloseDoor() {
  bool closed = false;
  unsigned long current = micros();
  //Serial.println("close door");
  while(!closed){
    if(current-last_manual_time>=600000){ 
      break;
    }
    if (digitalRead(doorClosedSwitch) == LOW)
    {
     //the switch is depressed so door is closed
      closed = true;
    }else{
      StepDown();    
    } 
  }
  while(digitalRead(doorClosedSwitch) == LOW) // step up to release switch to save power
  {
    StepUp();
  }
  last_door_change_time = micros();
  EEPROM.write(2, false);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);   
}

void OpenDoor() {
  bool isOpen = false;
  unsigned long current = micros();
  //Serial.println("open door");
  while(!isOpen){
    if(current-last_manual_time>=600000){ 
      break;
    }
    if (digitalRead(doorOpenSwitch) == LOW)
    {
     //the switch is depressed so door is closed
     isOpen = true;
    }else{
    StepUp();
    //myStepper.step(revolutionsToOpen * 200);
    
    } 
  }
  while(digitalRead(doorOpenSwitch) == LOW) // step down to release switch to save power
  {
    StepDown();
  }
  last_door_change_time = micros();
  EEPROM.write(2, true);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);   
}

void StepUp() {
  Direction = UP;
  //myStepper.step(-200);
  moveSteps(stepsPerRevolution);
}
void StepDown() {
  Direction = DOWN;
  moveSteps(stepsPerRevolution);
  //myStepper.step(200);
}

/*
void MenuLoop() {
  // Serial.println("Menu loop");
  Display("press 1 to calibrate door", "press 2 to calibrate sensor");

  delay(1000);
  long starttime = millis();
  long endtime = starttime;
  while ((endtime - starttime) <= 10000) // do this loop for up to 1000mS
  {
    String key = GetKeyPress();
    // Serial.println("Menu loop key " + key);
    if (key == "1") {
      CalibrateDoor();
    } else if (key == "2") {
      CalibrateSensor();
    } else {
      delay(500);
    }
    endtime = millis();
  }
}*/

void CheckForInput()  
{
  // Serial.println("Check for input");
  char customKey = customKeypad.getKey();
  bool doorIsOpen;
  EEPROM.get(2, doorIsOpen);
  if (customKey)
  {
    if (customKey == '3') {
      if (!doorIsOpen) {
        last_manual_time = micros();
        OpenDoor();
      }
    }
    if (customKey == '4') {
      if (doorIsOpen) { 
        last_manual_time = micros();
        CloseDoor();
      }
    }
  }
}

/*
void Display(String val) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(val);
  display.setTextColor(WHITE, BLACK);
  display.display();
}

void Display(String val, int line) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(val);
  display.setTextColor(WHITE, BLACK);
  display.display();
}

void Display(String line1, String line2) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(line1);
   display.println(line2);
  display.setTextColor(WHITE, BLACK);
  display.display();
}
*/
/*
String GetKeyPress() {
  char customKey = customKeypad.getKey();
  if (customKey)
  {
    // Serial.println("Key press");
    // Serial.println(customKey);
    if (customKey == '3') {
      return "UP";
    }
    if (customKey == '4') {
      return "DOWN";
    }
    if (customKey == '0') {
      return "BACK";
    }
    return String(customKey);
  }
  return "";
}
*/

/*
void CalibrateSensor() {
  // Serial.println("calibration Sensor");
  Display("Current light level: " + String(analogRead(photocellPin)));
  delay(2000);
  Display("Thershold used to open door");
  delay(2000);
  Display("Press 'Up' or 'Down' to adjust.");
  delay(2000);
  Display("Press '1' to save value");
  delay(2000);
  Display("Press '1' to save value");
  int level = EEPROMReadInt(3);
  Display("Level " + String(level));
  long starttime = millis();
  long endtime = starttime;
  String key = GetKeyPress();
  while ((endtime - starttime) <= 10000) // loop until no 'up' or down press for 3 seconds
  {
    String newKey = GetKeyPress();
    if (newKey != "") {
      key = newKey;
    }
    KeyState state = customKeypad.getState();
    // Serial.println(key);
    if ((key == "UP") && (state != IDLE)) {
      starttime = millis() - 7000;
      // Serial.println("threshold up");
      level++;
      Display("Level " + String(level));
    } else if ((key == "DOWN") && (state != IDLE)) {
      starttime = millis() - 7000;
      // Serial.println("threshold down");
      level--;
      Display("Level " + String(level));
    } else if (key == "BACK") {
      return;
    } else if (key == "1") {
      // Serial.println("threshold saved");
      EEPROMWriteInt(3, level);
      sensorLimit = level;
      Display("Limit saved at level " + String(level));
      delay(2000);
      return;
    }
    else {
      // Serial.println("waiting 100");
      delay(100);
    }
    endtime = millis();
  }
}
*/
/*
void CalibrateDoor() {
  // Serial.println("calibration");
  Display("Press up until open");
  long starttime = millis();
  long endtime = starttime;
  String key = GetKeyPress();

  while ((endtime - starttime) <= 10000) // loop until no 'up' press for 3 seconds
  {
    String newKey = GetKeyPress();
    if (newKey != "") {
      key = newKey;
    }
    KeyState state = customKeypad.getState();
    // Serial.println(key);
    if ((key == "UP") && (state != IDLE)) {
      starttime = millis() - 7000;
      // Serial.println("Step up");
      StepUp();
    } else if (key == "BACK") {
      return;
    }
    else {
      // Serial.println("waiting 100");
      delay(100);
    }
    endtime = millis();
  }
  int stepsToOpen = 0;
  Display("Press down until closed");
  starttime = millis();
  endtime = starttime;
  while ((endtime - starttime) <= 10000) // loop until no 'up' press for 3 seconds
  {
    String newKey = GetKeyPress();
    if (newKey != "") {
      key = newKey;
    }
    KeyState state = customKeypad.getState();
    // Serial.println(key);
    if ((key == "DOWN") && (state != IDLE)) {
      starttime = millis() - 7000;
      StepDown();
      stepsToOpen++;
    } else if (key == "BACK") {
      return;
    } else {
      delay(100);
    }
    endtime = millis();
  }
  if (stepsToOpen > 0) {
    // Serial.println("Updating steps to open");
    // Serial.println(stepsToOpen);
    EEPROMWriteInt(0, stepsToOpen);
    EEPROM.write(2, false);
  }
  Display("Calibration complete");
  delay(5000);
  OpenDoor();
}
*/

void EEPROMWriteInt(int address, int value)
{
  byte two = (value & 0xFF);
  byte one = ((value >> 8) & 0xFF);

  EEPROM.update(address, two);
  EEPROM.update(address + 1, one);
}

int EEPROMReadInt(int address)
{
  long two = EEPROM.read(address);
  long one = EEPROM.read(address + 1);

  return ((two << 0) & 0xFFFFFF) + ((one << 8) & 0xFFFFFFFF);
}

void moveSteps(long steps){
   //Serial.println("Move steps");
   //Serial.println(steps);
   while(steps>0){
    currentMillis = micros();
    if(currentMillis-last_time>=1000){ 
      moveStep(1); 
      time=time+micros()-last_time;
      last_time=micros();
      steps--;
    }
  }
}

void moveStep(int xw){
  
  for (int x=0;x<xw;x++){
switch(NextStep){
   case 0:
     digitalWrite(IN1, LOW); 
     digitalWrite(IN2, LOW);
     digitalWrite(IN3, LOW);
     digitalWrite(IN4, HIGH);
   break; 
   case 1:
     digitalWrite(IN1, LOW); 
     digitalWrite(IN2, LOW);
     digitalWrite(IN3, HIGH);
     digitalWrite(IN4, HIGH);
   break; 
   case 2:
     digitalWrite(IN1, LOW); 
     digitalWrite(IN2, LOW);
     digitalWrite(IN3, HIGH);
     digitalWrite(IN4, LOW);
   break; 
   case 3:
     digitalWrite(IN1, LOW); 
     digitalWrite(IN2, HIGH);
     digitalWrite(IN3, HIGH);
     digitalWrite(IN4, LOW);
   break; 
   case 4:
     digitalWrite(IN1, LOW); 
     digitalWrite(IN2, HIGH);
     digitalWrite(IN3, LOW);
     digitalWrite(IN4, LOW);
   break; 
   case 5:
     digitalWrite(IN1, HIGH); 
     digitalWrite(IN2, HIGH);
     digitalWrite(IN3, LOW);
     digitalWrite(IN4, LOW);
   break; 
     case 6:
     digitalWrite(IN1, HIGH); 
     digitalWrite(IN2, LOW);
     digitalWrite(IN3, LOW);
     digitalWrite(IN4, LOW);
   break; 
   case 7:
     digitalWrite(IN1, HIGH); 
     digitalWrite(IN2, LOW);
     digitalWrite(IN3, LOW);
     digitalWrite(IN4, HIGH);
   break; 
   default:
     digitalWrite(IN1, LOW); 
     digitalWrite(IN2, LOW);
     digitalWrite(IN3, LOW);
     digitalWrite(IN4, LOW);
   break; 
}
SetNextStep();
}
} 
void SetNextStep(){
if(Direction==1){ NextStep++;}
if(Direction==0){ NextStep--; }
if(NextStep>7){NextStep=0;}
if(NextStep<0){NextStep=7; }
}
