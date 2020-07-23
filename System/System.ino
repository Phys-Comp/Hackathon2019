#include <Adafruit_STMPE610.h>
#include <ESP8266WiFi.h>
#include <ILI9341_SPI.h>
#include <MiniGrafx.h>
#include "ArialRounded.h"
#include "SansSerif.h"
#include "Settings.h"

/********* SystemApp functions ********/
void closeSystemApp();
void openSystemApp();
void touchedSystemApp(TS_Point point);
void updateSystemApp();
/**************************************/

uint16_t palette[] = {ILI9341_BLACK, ILI9341_WHITE};

ADC_MODE(ADC_VCC);
ILI9341_SPI display = ILI9341_SPI(TFT_CS, TFT_DC);
MiniGrafx gfx = MiniGrafx(&display, 1, palette);
Adafruit_STMPE610 touchscreen = Adafruit_STMPE610(TOUCH_CS);

uint32_t lastTouchMillis = 0;

void setup() {
  //connecting wifi
  Serial.begin(115200);  
  Serial.print("\nConnecting to " + String(ssid));
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected.");
  Serial.println("IP address: " + WiFi.localIP().toString());

  //initializing touchscreen
  while (!touchscreen.begin()) {
    Serial.println("Error on starting touchscreen!");
    delay(5000);
  }
  //opening system app
  openSystemApp();
}

void loop() {
  //updating system app
  updateSystemApp();
  handleTouch();
}

//handles touch events
void handleTouch() {
  if (!touchscreen.bufferEmpty() && millis() - lastTouchMillis > 1000) {
    lastTouchMillis = millis();
    TS_Point point = touchscreen.getPoint();
    int x = display.width() - map(point.x, TS_MIN_X, TS_MAX_X, 0, display.width());
    int y = map(point.y, TS_MIN_Y, TS_MAX_Y, 0, display.height());
    point.x = (x < 0) ? 0 : (x > display.width()) ? display.width() : x;
    point.y = (y < 0) ? 0 : (y > display.height()) ? display.height() : y;
    
    Serial.println("----- TouchEvent ------");
    Serial.println("Point: ( " + String(point.x) + " | " + point.y + " )");
    touchedSystemApp(point);
    touchscreen.writeRegister8(STMPE_INT_STA, 0xFF);
  }
}
