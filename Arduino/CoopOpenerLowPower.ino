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

byte rowPins[ROWS] = {2};
byte colPins[COLS] = {5,6,3,4};

//const int stepsPerRevolution = 1100;  // change this to fit the number of steps per revolution
//const int stepDistance = 200;
int photocellPin = 0;     // the cell and 10K pulldown are connected to a0
int sensorLimit = 200;
/*-----( Declare objects )-----*/
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
// initialize the stepper library on pins 8 through 11:
int stepPin1 = 7;
int stepPin2 = 9;
int stepPin3 = 8;
int stepPin4 = 10;
Stepper myStepper(1100, stepPin1, stepPin2, stepPin3, stepPin4);

void setup() {
  //// Serial.begin(9600);
  // put your setup code here, to run once:
  myStepper.setSpeed(45);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  // init done
  display.setTextColor(WHITE, BLACK);
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();

  delay(2000);

  // Clear the buffer.
  display.clearDisplay();
  int stepsToOpen = EEPROMReadInt(0);
  // Serial.println("steps to open");
  // Serial.println(stepsToOpen);
  int sensorLevel = EEPROMReadInt(3);
  if (sensorLevel < 0) {
    EEPROMWriteInt(3, sensorLimit);
  } else {
    sensorLimit = sensorLevel;
  }
}

void loop() {
  // Serial.println("Main loop");
  // put your main code here, to run repeatedly:
  CheckLightSensor();
  CheckForInput();
  display.clearDisplay();
  display.display();
    // ATmega328P, ATmega168
  //LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, 
//                SPI_OFF, USART0_OFF, TWI_OFF);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  
}

void CheckLightSensor() {
  int photocellReading = analogRead(photocellPin);
  // Serial.println("photo cell");
  // Serial.println(photocellReading);
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
  delay(1000);
}

void CloseDoor() {
  // Serial.println("Close Door");
//  digitalWrite(stepPin1, HIGH);
//  digitalWrite(stepPin3, HIGH);
  int stepsToOpen = EEPROMReadInt(0);
  // Serial.println(stepsToOpen);
  myStepper.step(stepsToOpen * 200);
  EEPROM.write(2, false);
  digitalWrite(stepPin1, LOW);
  digitalWrite(stepPin2, LOW);
  digitalWrite(stepPin3, LOW);
  digitalWrite(stepPin4, LOW);
}

void OpenDoor() {
  // Serial.println("Open door");
 // digitalWrite(10, HIGH);
 // digitalWrite(11, HIGH);
  int stepsToOpen = EEPROMReadInt(0);
  // Serial.println(stepsToOpen);
  myStepper.step(-stepsToOpen * 200);
  EEPROM.write(2, true);
  digitalWrite(stepPin1, LOW);
  digitalWrite(stepPin2, LOW);
  digitalWrite(stepPin3, LOW);
  digitalWrite(stepPin4, LOW);
}

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
}

void CheckForInput()   /*----( LOOP: RUNS CONSTANTLY )----*/
{
  // Serial.println("Check for input");
  char customKey = customKeypad.getKey();
  bool doorIsOpen;
  EEPROM.get(2, doorIsOpen);
  if (customKey)
  {
    if (customKey == '3') {
      if (!doorIsOpen) {
        Display("Opening door");
        OpenDoor();
      }
    }
    if (customKey == '4') {
      if (doorIsOpen) {
        Display("Closing door");
        CloseDoor();
      }
    }
    else {
      MenuLoop();
    }
  }
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

void StepUp() {
  myStepper.step(-200);
}
void StepDown() {
  myStepper.step(200);
}

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
