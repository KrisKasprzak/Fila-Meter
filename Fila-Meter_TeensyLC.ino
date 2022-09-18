#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ST7735_Menu.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Colors.h>
#include "fonts\FreeSansBold12pt7b.h"
#include "fonts\FreeSans12pt7b.h"
#include <FlickerFreePrint.h>
#include <Encoder.h>

#define DATA_COL     85
#define DEBOUNCE    100
#define CS_PIN       10
#define RST_PIN       8
#define DC_PIN        9

#define SET_PIN 2
#define CNT_PIN 3
#define ALM_PIN 6
#define EN1_PIN 5
#define EN2_PIN 7

float Dia = 0.03289;      // diameter of counter wheel (in meters)
int CPR = 10;             // number of pickups on counter wheel
float RollCost = 25.0;    // what I pay for a full 330 meter roll

float Cost = 0.0;
float FullRoll = 330.0;   // standard length of 1.75mm spool
float Remaining = 330.0;  // standard length of 1.75mm spool
float Used = 0.0, OldUsed = 0.0;
float LowLevelLimit = 10.0;
long newPosition = 0;
long oldPosition  = -999;

unsigned long LowTime = 0;
unsigned long LowAlarmLevel = 0;
unsigned long DoneTime = 20;

bool LowPulse = false;
bool LowLevel = false;
bool LevelLimit = false;

int Option1 = 0, Option2 = 0, Option3 = 0, Option4 = 0, Option5 = 0;

elapsedMillis dt;

Adafruit_ST7735 Display = Adafruit_ST7735(CS_PIN, DC_PIN, RST_PIN);

FlickerFreePrint<Adafruit_ST7735> ffLeft(&Display, C_WHITE, C_BLACK);
FlickerFreePrint<Adafruit_ST7735> ffUsed(&Display, C_WHITE, C_BLACK);
FlickerFreePrint<Adafruit_ST7735> ffCost(&Display, C_WHITE, C_BLACK);

EditMenu OptionMenu(&Display);


Encoder InputEnc(EN1_PIN, EN2_PIN);

void setup(void) {

  Serial.begin(9600);

  pinMode(SET_PIN, INPUT_PULLUP);
  pinMode(CNT_PIN, INPUT_PULLUP);
  pinMode(ALM_PIN, OUTPUT);
  Display.initR(INITR_BLACKTAB);      // Init ST7735S chip, green tab
  Display.setSPISpeed(60000000);
  Display.setRotation(1);

  analogWriteFrequency(ALM_PIN, 2000);

  GetParameters();

  OptionMenu.init(C_WHITE, C_BLACK, C_WHITE, C_DKBLUE, C_WHITE, C_DKRED,
                  68, 30, 3, "Options", FreeSans12pt7b, FreeSansBold12pt7b);

  Option1 = OptionMenu.addNI("Len.", Remaining, 0, 350, 1, 0, NULL);
  Option2 = OptionMenu.addNI("Dia.", Dia, .02, .04, .00001, 5, NULL);
  Option3 = OptionMenu.addNI("Low", LowLevelLimit, 0, 20, 1, 0, NULL);
  Option4 = OptionMenu.addNI("Done", DoneTime, 0, 20, 1, 0, NULL);
  Option5 = OptionMenu.addNI("Cost", RollCost, 20, 40, .5, 1, NULL);

  // display is green tab, but colors get
  // Display.initR(INITR_GREENTAB);
  // change .cpp line 240 and 241
  //  _colstart = 2;
  //  _rowstart = 1;

  analogWrite(ALM_PIN, 127);
  delay(100);
  analogWrite(ALM_PIN, 0);
  delay(100);
  analogWrite(ALM_PIN, 127);
  delay(100);
  analogWrite(ALM_PIN, 0);

  DrawHeader();
  UpdateData();
  dt = 0;

}

void loop() {

  ProcessButton();

  // spool running low?
  if (Remaining < LowLevelLimit) {
    LevelLimit = true;
    if ((millis() - LowAlarmLevel) > 1000) {
      LowAlarmLevel = millis();
      LowLevel = !LowLevel;
    }
    if (LowLevel) {
      analogWrite(ALM_PIN, 200);
    }
    else {
      analogWrite(ALM_PIN, 0);
    }
  }

  // save every meter
  if ((Used - OldUsed) > 1.0) {
    // save every meter no matter what
    OldUsed = Used;
    EEPROM.put(0, Remaining);
    analogWrite(ALM_PIN, 127);
    delay(100);
    analogWrite(ALM_PIN, 0);
  }

  // done when no activity after a certian time
  if (Used == OldUsed) {
    if ((DoneTime > 0) & (dt > (DoneTime * 60000))) {
      analogWrite(ALM_PIN, 127);
    }
  }
  else {
    dt = 0;
  }

  // if signal is low start a watch
  if (digitalRead(CNT_PIN) == LOW) {
    LowTime = millis();
    LowPulse = true;
  }

  // if signal then goes high, consider it a hit
  if (LowPulse && (digitalRead(CNT_PIN) == HIGH)) {
    if (millis() - LowTime > 1000) {
      LowPulse = false;
      LowTime = millis();
      //Click++;
      Used = Used + ((Dia * 3.1415927) / CPR);
      Remaining = Remaining - ((Dia * 3.1415927) / CPR);
      Cost = Used * RollCost / FullRoll;
      UpdateData();

    }
  }
}

void ProcessButton() {

  if (digitalRead(SET_PIN) == LOW) {

    ProcessOptionMenu();
    DrawHeader();
    UpdateData();

  }

}
void UpdateData() {

  Display.setFont(&FreeSansBold12pt7b);
  Display.setTextColor(C_WHITE);

  Display.setCursor(DATA_COL, 55);
  ffLeft.setTextColor(C_WHITE, C_BLACK);
  ffLeft.print(Remaining, 2);

  Display.setCursor(DATA_COL, 85);
  ffUsed.setTextColor(C_WHITE, C_BLACK);
  ffUsed.print(Used, 2);

  Display.setCursor(DATA_COL, 115);
  ffCost.setTextColor(C_WHITE, C_BLACK);
  ffCost.print(Cost, 2);

}

void DrawHeader() {

  Display.fillScreen(C_BLACK);
  Display.fillRect(0, 0, 160, 30, C_DKCYAN);

  Display.setFont(&FreeSansBold12pt7b);
  Display.setTextColor(C_WHITE);
  Display.setCursor(10, 20);
  Display.println(F("Fila - Meter"));

  Display.setFont(&FreeSans12pt7b);

  Display.setTextColor(C_WHITE);
  Display.setCursor(10, 55);
  Display.print(F("Left"));
  Display.setCursor(10, 85);
  Display.print(F("Used"));
  Display.setCursor(10, 115);
  Display.print(F("Cost $"));

}

void GetParameters() {

  if (digitalRead(SET_PIN) == LOW) {
    Remaining = FullRoll;
    EEPROM.put(0, Remaining);
    EEPROM.put(10, Dia);
    EEPROM.put(20, LowLevelLimit);
    EEPROM.put(30, DoneTime);
    EEPROM.put(40, RollCost);

  }

  EEPROM.get(0, Remaining);
  EEPROM.get(10, Dia);
  EEPROM.get(20, LowLevelLimit);
  EEPROM.get(30, DoneTime);
  EEPROM.get(40, RollCost);

  delay(100);

}

void ProcessOptionMenu() {

  bool jump = false;
  long jj = 0;
  long Position = 0 , oldPosition = 0;
  int EditMenuOption = 1;

  analogWriteFrequency(ALM_PIN, 6000);

  // blank out the screen
  Display.fillScreen(C_BLACK);

  OptionMenu.setItemValue(Option1, Remaining);

  // draw the main menu
  OptionMenu.draw();

  while (EditMenuOption != 0) {

    // standard encoder read
    Position = InputEnc.read();


    if (abs(jj - Position) > 5) {
      OptionMenu.setIncrement(Option1, 20);
      jump = true;
    }
    else {
      if (jump) {
        OptionMenu.setIncrement(Option1, 1);
        jump = false;
      }
    }

    // attempt to debouce these darn things...
    if ((oldPosition - Position ) > 0) {
      delay(DEBOUNCE);
      while (oldPosition != Position) {
        oldPosition = Position;
        Position = InputEnc.read();
      }

      analogWrite(ALM_PIN, 127);
      delay(5);
      analogWrite(ALM_PIN, 0);
      OptionMenu.MoveUp();
    }

    if ((oldPosition - Position) < 0) {
      delay(DEBOUNCE);
      while (oldPosition != Position) {
        oldPosition = Position;
        Position = InputEnc.read();
      }

      analogWrite(ALM_PIN, 127);
      delay(5);
      analogWrite(ALM_PIN, 0);
      OptionMenu.MoveDown();
    }

    if (digitalRead(SET_PIN) == LOW) {

      // debounce the selector button
      while (digitalRead(SET_PIN) == LOW) {
        delay(DEBOUNCE);
      }


      EditMenuOption = OptionMenu.selectRow();

    }

    jj = Position;
  }

  Remaining = OptionMenu.value[Option1];
  Dia = OptionMenu.value[Option2];
  LowLevelLimit = OptionMenu.value[Option3];
  DoneTime = OptionMenu.value[Option4];
  RollCost = OptionMenu.value[Option5];

  EEPROM.put(0, Remaining);
  EEPROM.put(10, Dia);
  EEPROM.put(20, LowLevelLimit);
  EEPROM.put(30, DoneTime);
  EEPROM.put(40, RollCost);

  analogWriteFrequency(ALM_PIN, 2000);

  delay(100);

}
