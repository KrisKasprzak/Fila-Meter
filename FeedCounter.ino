#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Colors.h>
#include "fonts\FreeSansBold12pt7b.h"
#include "fonts\FreeSans12pt7b.h"
#include <FlickerFreePrint.h>

#define DATA_COL 85

#define CS_PIN        10
#define RST_PIN        8
#define DC_PIN         9

#define SET_PIN 2
#define CNT_PIN 3
#define ALM_PIN 6

float Dia = 0.03286;      // diameter of counter wheel (in meters)
int CPR = 10;             // number of pickups on counter wheel
float RollCost = 25.0;    // what I pay for a full 330 meter roll

float Cost = 0.0;
float FullRoll = 330.0;   // standard length of 1.75mm spool
float Remaining = 330.0;  // standard length of 1.75mm spool
float Used = 0.0, OldUsed = 0.0;
float x;
float LowLevelLimit = 10.0;
long newPosition = 0;
long oldPosition  = -999;
long Click  = 0;
unsigned long LowTime = 0;
unsigned long PressTime = 0;
unsigned long LowLevelAlarm = 0;

bool Saved = false;
bool Started = false;
bool Low = false;
bool LowLevel = false;
bool LevelLimit = false;

Adafruit_ST7735 Display = Adafruit_ST7735(CS_PIN, DC_PIN, RST_PIN);

FlickerFreePrint<Adafruit_ST7735> ffLeft(&Display, C_WHITE, C_BLACK);
FlickerFreePrint<Adafruit_ST7735> ffUsed(&Display, C_WHITE, C_BLACK);
FlickerFreePrint<Adafruit_ST7735> ffCost(&Display, C_WHITE, C_BLACK);

void setup(void) {

  Serial.begin(9600);

  pinMode(SET_PIN, INPUT_PULLUP);
  pinMode(CNT_PIN, INPUT_PULLUP);
  pinMode(ALM_PIN, OUTPUT);

  GetParameters();

  // display is green tab, but colors get
  // Display.initR(INITR_GREENTAB);
  // change .cpp line 240 and 241
  //  _colstart = 2;
  //  _rowstart = 1;
  Display.initR(INITR_BLACKTAB);      // Init ST7735S chip, green tab
  Display.setSPISpeed(60000000);
  Display.setRotation(1);

  analogWrite(ALM_PIN, 127);
  delay(100);
  analogWrite(ALM_PIN, 0);
  delay(100);
  analogWrite(ALM_PIN, 127);
  delay(100);
  analogWrite(ALM_PIN, 0);

  DrawHeader();
  UpdateData();

}

void loop() {

  ProcessButton();

  // spool running low?
  if (Remaining < LowLevelLimit) {
    LevelLimit = true;
    if ((millis() - LowLevelAlarm) > 1000) {
      LowLevelAlarm = millis();
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

  // if signal is low start a watch
  if (digitalRead(CNT_PIN) == LOW) {
    LowTime = millis();
    Low = true;
  }

  // if signal then goes high, consider it a hit
  if (Low && (digitalRead(CNT_PIN) == HIGH)) {
    if (millis() - LowTime > 1000) {
      Started = true;
      Low = false;
      LowTime = millis();
      Click++;
      Used = Used + ((Dia * 3.1415927) / CPR);
      Remaining = Remaining - ((Dia * 3.1415927) / CPR);
      Cost = Used * RollCost / FullRoll;
      UpdateData();

    }
  }
}

void ProcessButton() {

  if (digitalRead(SET_PIN) == LOW) {

    PressTime = millis();

    while (digitalRead(SET_PIN) == LOW) {
      delay(100);
    }
    PressTime = millis() - PressTime;

    if (PressTime < 1000) {

      if (LevelLimit) {
        LevelLimit = false;
        analogWrite(ALM_PIN, 0);
        LowLevelLimit -= 1.0;
      }
      else {
        Display.fillRect(30, 40, 120, 60, C_RED);
        Display.setFont(&FreeSansBold12pt7b);
        Display.setTextColor(C_WHITE);
        Display.setCursor(50, 60);
        Display.println(F("Spool"));
        Display.setCursor(50, 90);
        Display.println(F("Saved"));

        EEPROM.put(0, Remaining);
        analogWrite(ALM_PIN, 127);
        delay(100);
        analogWrite(ALM_PIN, 0);

        delay(3000);
        Display.fillScreen(C_BLACK);
        DrawHeader();
        UpdateData();
      }
    }
    else {
      // reset roller
      // new roll
      Display.fillRect(30, 40, 120, 60, C_RED);
      Display.setFont(&FreeSansBold12pt7b);
      Display.setTextColor(C_WHITE);
      Display.setCursor(50, 60);
      Display.println(F("New"));
      Display.setCursor(50, 90);
      Display.println(F("Spool"));

      Remaining = FullRoll;
      Used = 0.0;
      Cost = 0.0;

      EEPROM.put(0, Remaining);
      analogWrite(ALM_PIN, 127);
      delay(100);
      analogWrite(ALM_PIN, 0);
      delay(100);
      analogWrite(ALM_PIN, 127);
      delay(100);
      analogWrite(ALM_PIN, 0);

      delay(3000);
      Display.fillScreen(C_BLACK);
      DrawHeader();
      UpdateData();
    }

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
  Display.fillRect(0, 0, 160, 30, C_DKBLUE);

  Display.setFont(&FreeSansBold12pt7b);
  Display.setTextColor(C_WHITE);
  Display.setCursor(10, 20);
  Display.println(F("Fila-Minder"));

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
    // new roll
    Remaining = FullRoll;
    EEPROM.put(0, Remaining);
  }

  EEPROM.get(0, Remaining);

}
