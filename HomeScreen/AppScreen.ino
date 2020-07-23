#include <FS.h>
#include "HomeIcons.h"

#define HOME_BLACK 0
#define HOME_WHITE 1
#define HOME_RED 2
#define HOME_GRAY 3

#define NTP_MIN_VALID_EPOCH 1533081600
#define NTP_SERVERS "0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org"

#define VOLTAGE_GAUGE 0.40

simpleDSTadjust dstAdjusted(StartRule, EndRule);
uint32_t lastTimeUpdate = 0;
time_t now;

uint16_t alarmPalette[] = {ILI9341_BLACK,  // 0 -> RGB888: #000000
                           ILI9341_WHITE,  // 1 -> RGB888: #FFFFFF
                           0xB9A6,         // 2 -> RGB888: #B83737
                           0x840F          // 3 -> RGB888: #808078
};
uint16_t homePalette[] = {ILI9341_BLACK,  // 0 -> RGB888: #000000
                          ILI9341_WHITE,  // 1 -> RGB888: #FFFFFF
                          0xF1E4,         // 2 -> RGB888: #F53F20
                          0x840F          // 3 -> RGB888: #808078
};
uint16_t musicPalette[] = {ILI9341_BLACK,  // 0 -> RGB888: #000000
                           ILI9341_WHITE,  // 1 -> RGB888: #FFFFFF
                           0x89F4,         // 2 -> RGB888: #8C3CA0
                           0xB5B5          // 3 -> RGB888: #B4B4AA
};
uint16_t newsPalette[] = {ILI9341_BLACK,  // 0 -> RGB888: #000000
                          ILI9341_WHITE,  // 1 -> RGB888: #FFFFFF
                          0x046F,         // 2 -> RGB888: #008C78
                          0x1CFE          // 3 -> RGB888: #1B9CF7
};
uint16_t radioPalette[] = {ILI9341_BLACK,  // 0 -> RGB888: #000000
                           ILI9341_WHITE,  // 1 -> RGB888: #FFFFFF
                           0xF446,         // 2 -> RGB888: #F78837
                           0x632C,         // 3 -> RGB888: #646464
};
uint16_t systemPalette[] = {ILI9341_BLACK,  // 0 -> RGB888: #000000
                            ILI9341_WHITE,  // 1 -> RGB888: #FFFFFF
                            0xB5B5,         // 2 -> RGB888: #B4B4AA
                            0x840F          // 3 -> RGB888: #808078
};
uint16_t spotifyPalette[] = {ILI9341_BLACK,  // 0 -> RGB888: #000000
                             ILI9341_WHITE,  // 1 -> RGB888: #FFFFFF
                             0x368B,         // 2 -> RGB888: #32D25A
                             0x840F          // 3 -> RGB888: #808078
};
uint16_t weatherPalette[] = {ILI9341_BLACK,   // 0 -> RGB888: #000000
                             ILI9341_WHITE,   // 1 -> RGB888: #FFFFFF
                             ILI9341_YELLOW,  // 2 -> RGB888: #FFFF00
                             0x7E3C           // 3 -> RGB888: #78C4E0
};
int lastUpdate = 0;

//opens and loads the app screen
void openAppScreen() {
  gfx = MiniGrafx(&display, BITS_PER_PIXEL, homePalette, display.width(), display.height());
  gfx.init();
  gfx.fillBuffer(HOME_BLACK);
  gfx.commit(0, 0);
  
  //loading display
  drawProgress(10, 100, "Loading display...", HOME_BLACK, HOME_WHITE, HOME_GRAY, HOME_WHITE);
  delay(500);

  //updating time
  drawProgress(50, 100, "Updating time...", HOME_BLACK, HOME_WHITE, HOME_GRAY, HOME_WHITE);
  updateTime();
  now = dstAdjusted.time(NULL);

  //finished setup
  drawProgress(100, 100, "Finished!", HOME_BLACK, HOME_WHITE, HOME_GRAY, HOME_WHITE);
  delay(500);
  gfx.fillBuffer(0);
  gfx.commit();
  drawBackground();
}

//updates the app screen
void updateAppScreen() {
  now = dstAdjusted.time(NULL);

  //updating app screen title
  if(millis() % 50 == 0) {
    drawScreenTitle();
  }
  
  //updating time
  if(millis() - lastTimeUpdate > UPDATE_INTERVAL) {
    updateTime();
    lastTimeUpdate = millis();
  }
}

//draws the app screen background
void drawBackground() {
  //drawing octopus logo
  gfx = MiniGrafx(&display, 1, palette, display.width(), display.height() - 18);
  gfx.init();
  gfx.fillBuffer(0);
  gfx.drawPalettedBitmapFromPgm(0, 0, background);
  gfx.commit(0, 18);
  
  //settings for app names
  gfx.setColor(1);
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  
  //drawing system app logo
  gfx = MiniGrafx(&display, BITS_PER_PIXEL, systemPalette, 32, 32);
  gfx.init();
  gfx.drawPalettedBitmapFromPgm(0, 0, systemLogo);
  gfx.commit(14, 52);
  
  //drawing msuic app logo
  gfx = MiniGrafx(&display, BITS_PER_PIXEL, musicPalette, 32, 32);
  gfx.init();
  gfx.drawPalettedBitmapFromPgm(0, 0, musicLogo);
  gfx.commit(14, 120);
  
  //drawing radio app logo
  gfx = MiniGrafx(&display, BITS_PER_PIXEL, radioPalette, 32, 32);
  gfx.init();
  gfx.drawPalettedBitmapFromPgm(0, 0, radioLogo);
  gfx.commit(14, 188);
  
  //drawing alarm app logo
  gfx = MiniGrafx(&display, BITS_PER_PIXEL, alarmPalette, 32, 32);
  gfx.init();
  gfx.drawPalettedBitmapFromPgm(0, 0, alarmLogo);
  gfx.commit(14, 256);
  
  //drawing news app logo
  gfx = MiniGrafx(&display, BITS_PER_PIXEL, newsPalette, 32, 32);
  gfx.init();
  gfx.drawPalettedBitmapFromPgm(0, 0, newsLogo);
  gfx.commit(74, 256);
  
  //drawing spotify app logo
  gfx = MiniGrafx(&display, BITS_PER_PIXEL, spotifyPalette, 32, 32);
  gfx.init();
  gfx.drawPalettedBitmapFromPgm(0, 0, spotifyLogo);
  gfx.commit(134, 256);
  
  //drawing weather app logo
  gfx = MiniGrafx(&display, BITS_PER_PIXEL, weatherPalette, 32, 32);
  gfx.init();
  gfx.drawPalettedBitmapFromPgm(0, 0, weatherLogo);
  gfx.commit(194, 256);
  
  gfx = MiniGrafx(&display, BITS_PER_PIXEL, homePalette, display.width() / 2, 18);
  gfx.init();
}

void drawScreenTitle() {
    //drawing app screen title
    gfx.fillBuffer(HOME_BLACK);
    gfx.setColor(HOME_WHITE);
    gfx.setFont(ArialRoundedMTBold_16);
    gfx.setTextAlignment(TEXT_ALIGN_LEFT);
    gfx.drawString(5, 0, getTime(&now, true));
    gfx.commit(0, 0);

    if(millis() - lastUpdate > 1000) {
      gfx.fillBuffer(HOME_BLACK);
      
      //drawing voltage icon
      double voltage = (ESP.getVcc() / 1024.0) + VOLTAGE_GAUGE;
      if(voltage > 3.2)
        gfx.drawPalettedBitmapFromPgm(103, 0, battery4);
      else if(voltage > 3.1)
        gfx.drawPalettedBitmapFromPgm(103, 0, battery3);
      else if(voltage > 3.0)
        gfx.drawPalettedBitmapFromPgm(103, 0, battery2);
      else
        gfx.drawPalettedBitmapFromPgm(103, 0, battery1);
  
      //drawing wifi quality icon
      uint8_t wifiQuality = getWiFiQuality();
      if(WiFi.status() != WL_CONNECTED || wifiQuality == 0)
        gfx.drawPalettedBitmapFromPgm(80, 0, wlan0);
      else if(wifiQuality > 75)
        gfx.drawPalettedBitmapFromPgm(80, 0, wlan4);
      else if(wifiQuality > 50)
        gfx.drawPalettedBitmapFromPgm(80, 0, wlan3);
      else if(wifiQuality > 25)
        gfx.drawPalettedBitmapFromPgm(80, 0, wlan2);
      else if(wifiQuality > 0)
        gfx.drawPalettedBitmapFromPgm(80, 0, wlan1);
        
      //drawing wifi quality
      gfx.setColor(HOME_WHITE);
      gfx.setFont(ArialRoundedMTBold_14);
      gfx.setTextAlignment(TEXT_ALIGN_RIGHT);
      gfx.drawString(77, 1, String(wifiQuality) + "%");
      
      gfx.commit(120, 0);
      lastUpdate = millis();
    }
}

//updates the time
void updateTime() {
 configTime(UTC_OFFSET * 3600, 0, NTP_SERVERS);
  while((now = time(NULL)) < NTP_MIN_VALID_EPOCH) {
    Serial.print(".");
    delay(300);
  }
}

//handles touch actions for app screen
void touchedAppScreen(TS_Point point) {
  
}

//gets formatted time
String getTime(time_t *timestamp, boolean seconds) {
  struct tm *timeInfo = gmtime(timestamp);
  if(seconds) {
    char buff[9];
    sprintf(buff, "%02d:%02d:%02d", timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
    return String(buff);
  } else {
    char buff[6];
    sprintf(buff, "%02d:%02d", timeInfo->tm_hour, timeInfo->tm_min);
    return String(buff);    
  }
}

int getWiFiQuality() {
  uint32_t dBm = WiFi.RSSI();

  return (dBm <= -100) ? 0 : (dBm >= -50) ? 100 : 2 * (dBm + 100);
}

//closes the app screen
void closeAppScreen() {
  gfx.fillBuffer(HOME_BLACK);
  gfx.commit(0, 18);
  gfx.fillBuffer(HOME_BLACK);
  gfx.commit(0, 0);
}
