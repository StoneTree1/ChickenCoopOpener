#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Stepper.h>
#include <Keypad.h>
#include <EEPROM.h>
#include "LowPower.h"
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define UP 0
#define DOWN 1
#define IN1  13
#define IN2  12
#define IN3  11
#define IN4  10
int NextStep = 0;
long stepsPerRevolution = 2048;
int revolutionsToOpen = 1;
boolean Direction = true;
unsigned long last_time;
unsigned long currentMillis ;
unsigned long last_door_change_time;
unsigned long last_sensor_check_time;
long time;
char lastKey;

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000
};

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

const byte ROWS = 1;
const byte COLS = 4;

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', '4'}
};

byte colPins[COLS] = {6, 5,8,7};
byte rowPins[ROWS] = {9};

//const int stepsPerRevolution = 1100;  // change this to fit the number of steps per revolution
//const int stepDistance = 200;
int photocellPin = 0;     // the cell and 10K pulldown are connected to a0
int sensorLimit = 200;
/*-----( Declare objects )-----*/
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
// initialize the stepper library on pins 8 through 11:

//Stepper myStepper(1100, stepPin1, stepPin2, stepPin3, stepPin4);

void setup() {
pinMode(IN1, OUTPUT); 
pinMode(IN2, OUTPUT); 
pinMode(IN3, OUTPUT); 
last_door_change_time = 0;
last_sensor_check_time = 0;

   // //Serial.begin(9600);
  // put your setup code here, to run once:
  //myStepper.setSpeed(40);
    display.setRotation(2); 
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  // init done
 
  display.setTextColor(WHITE, BLACK);
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();

  delay(2000);
  display.setRotation(2); 
  // Clear the buffer.
  display.clearDisplay();
  revolutionsToOpen = EEPROMReadInt(0);
    //Serial.println("revolutions to open");
    //Serial.println(revolutionsToOpen);
  int sensorLevel = EEPROMReadInt(3);
  if (sensorLevel < 0) {
    EEPROMWriteInt(3, sensorLimit);
  } else {
    sensorLimit = sensorLevel;
  }
}

void loop() {
  //  //Serial.println("Main loop");
  // put your main code here, to run repeatedly:
  CheckLightSensor();
  CheckForInput();
  display.clearDisplay();
  display.display();
  display.ssd1306_command(SSD1306_DISPLAYOFF); 
    
    // ATmega328P, ATmega168
  //LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, 
//                SPI_OFF, USART0_OFF, TWI_OFF);
  LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);  
  CheckForInput();
  LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);  
  CheckForInput();
  LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);  
  display.ssd1306_command(SSD1306_DISPLAYON);
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
      //delay(1000);
    }
  }
}

void CloseDoor() {
    //Serial.println("Close Door");
//  digitalWrite(stepPin1, HIGH);
//  digitalWrite(stepPin3, HIGH);
  //int revolutionsToOpen = EEPROMReadInt(0);
  //  //Serial.println(stepsToOpen);
  Direction=DOWN;
  moveSteps(revolutionsToOpen*stepsPerRevolution);
  last_door_change_time = micros();
  //myStepper.step(revolutionsToOpen * 200);
  EEPROM.write(2, false);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void OpenDoor() {
    //Serial.println("Open door");
 // digitalWrite(10, HIGH);
 // digitalWrite(11, HIGH);
 Display("Revolutions to open");
 delay(1000);
 Display(String(revolutionsToOpen));
 delay(1000);
  //revolutionsToOpen = EEPROMReadInt(0);
   //Serial.println("revolutions to open");
    //Serial.println(revolutionsToOpen);
  Direction=UP;
  long number = revolutionsToOpen*stepsPerRevolution;
  Display(String(stepsPerRevolution));
  delay(1000);
  Display("Steps to open "+ String(number));
  moveSteps(revolutionsToOpen*stepsPerRevolution);
  last_door_change_time = micros();
  //myStepper.step(-stepsToOpen * 200);
  EEPROM.write(2, true);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void MenuLoop() {
  //  //Serial.println("Menu loop");
  Display("press 1 to calibrate door", "press 2 to calibrate sensor");

  delay(1000);
  long starttime = millis();
  long endtime = starttime;
  while ((endtime - starttime) <= 10000) // do this loop for up to 1000mS
  {
    String key = GetKeyPress();
    //  //Serial.println("Menu loop key " + key);
    if (key == "1") {
      CalibrateDoor();
    } else if (key == "2") {
      CalibrateSensor();
    } else {
      delay(500);
    }
    endtime = millis();
  }
}

void CheckForInput()  
{
    //Display("Check for input");
  char customKey = customKeypad.getKey();
  char openKey = '3';
  bool doorIsOpen;
  EEPROM.get(2, doorIsOpen);
  if (customKey)
  {
     //Serial.println(customKey);
    if (customKey == openKey) {
      if (!doorIsOpen) {
        Display("Opening door");
        OpenDoor();
      }else{
         Display("Door already open");
         delay(2000);
         ClearDisplay();
      }
    }
    else if (customKey == '4') {
      if (doorIsOpen) {
        Display("Closing door");
        CloseDoor();
      }
      else{
         Display("Door already closed");
         delay(2000);
         
      }
    }
    else {
      MenuLoop();
    }
  }
}

String GetKeyPress()   /*----( LOOP: RUNS CONSTANTLY )----*/
{
  char customKey = customKeypad.getKey();
  if (customKey == '1')
  {
    lastKey = '1';
    return "1";
  }
  else if (customKey == '2') {
    lastKey = '2';
     return "2";
    }
  else if (customKey == '3') {
    lastKey = '3';
     return "UP";
    }
  else if (customKey == '4') {
    lastKey = '4'; 
    return "DOWN";
  }else{
       KeyState state = customKeypad.getState();
      if(0 < state ){
        if(state<3){
         if(lastKey == '3'){
          return "UP";
         }
         if(lastKey == '4'){
          return "DOWN";
         }
         return ""+lastKey;
        }
      }
    }
    return "";
}
/*
String GetKeyPress() {
  char customKey = customKeypad.getKey();
  if (customKey)
  {
    //  //Serial.println("Key press");
    //  //Serial.println(customKey);
    if (customKey == '3') {
      
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

void ClearDisplay(){
  display.clearDisplay();
  display.display();
}

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

void CalibrateSensor() {
  //  //Serial.println("calibration Sensor");
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
    //  //Serial.println(key);
    if ((key == "UP") && (state != IDLE)) {
      starttime = millis() - 7000;
      //  //Serial.println("threshold up");
      level++;
      Display("Level " + String(level));
    } else if ((key == "DOWN") && (state != IDLE)) {
      starttime = millis() - 7000;
      //  //Serial.println("threshold down");
      level--;
      Display("Level " + String(level));
    } else if (key == "BACK") {
      return;
    } else if (key == "1") {
      //  //Serial.println("threshold saved");
      EEPROMWriteInt(3, level);
      sensorLimit = level;
      Display("Limit saved at level " + String(level));
      delay(2000);
      return;
    }
    else {
      //  //Serial.println("waiting 100");
      delay(100);
    }
    endtime = millis();
  }
}

void CalibrateDoor() {
    //Serial.println("calibration");
  Display("Press up until open");
  long starttime = millis();
  long endtime = starttime;
  String key = GetKeyPress();

  while ((endtime - starttime) <= 20000) // loop until no 'up' press for 3 seconds
  {
    String newKey = GetKeyPress();
    if (newKey != "") {
      key = newKey;
    }
    KeyState state = customKeypad.getState();
     // //Serial.println(newKey);
     // //Serial.println(state);
    if ((key == "UP") && (state != IDLE)) {
      starttime = millis() - 15000;
        //Serial.println("Step up");
      StepUp();
    } else if (key == "BACK") {
      return;
    }
    else {
       // //Serial.println("waiting 100");
      delay(100);
    }
    endtime = millis();
  }
  int stepsToOpen = 0;
  Display("Press down until closed");
  starttime = millis();
  endtime = starttime;
  revolutionsToOpen = 0;
  // //Serial.println("");
  while ((endtime - starttime) <= 20000) // loop until no 'up' press for 3 seconds
  {
     //Serial.println("Revolutions to open");
     //Serial.println(revolutionsToOpen);
    String newKey = GetKeyPress();
    if (newKey != "") {
      key = newKey;
    }
    KeyState state = customKeypad.getState();
    //  //Serial.println(key);
    if ((key == "DOWN") && (state != IDLE)) {
      starttime = millis() - 15000;
      StepDown();
      //  stepsToOpen++;
      revolutionsToOpen++;
    } else if (key == "BACK") {
      return;
    } else {
      delay(100);
    }
    endtime = millis();
  }
  if (revolutionsToOpen > 0) {
      //Serial.println("Updating revolutions to open");
      //Serial.println(revolutionsToOpen);
    EEPROMWriteInt(0, revolutionsToOpen);
    EEPROM.write(2, false);
  }
  Display("Calibration complete");
  delay(5000);
  OpenDoor();
}

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
