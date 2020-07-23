#include "SystemIcons.h"

#define SYSTEM_WIDTH 240
#define SYSTEM_HEIGHT 302

#define SYSTEM_BLACK 0
#define SYSTEM_WHITE 1
#define SYSTEM_LIGHTGRAY 2
#define SYSTEM_GRAY 3

#define VOLTAGE_GAUGE 0.40

uint16_t systemPalette[] = {ILI9341_BLACK,  // 0 -> RGB888: #000000
                            ILI9341_WHITE,  // 1 -> RGB888: #FFFFFF
                            0xB5B5,         // 2 -> RGB888: #B4B4AA
                            0x840F          // 3 -> RGB888: #808078
};

long lastUpdate = 0;

//opens and loads the system app
void openSystemApp() {
  gfx = MiniGrafx(&display, BITS_PER_PIXEL, systemPalette, SYSTEM_WIDTH, SYSTEM_HEIGHT);
  gfx.init();
  gfx.fillBuffer(SYSTEM_BLACK);
  gfx.commit(0, 0);
  drawTitleBar("System App", SYSTEM_GRAY, SYSTEM_WHITE, closeButton);
  
  //loading display
  drawProgress(40, 100, "Loading display...", SYSTEM_BLACK, SYSTEM_WHITE, SYSTEM_GRAY, SYSTEM_LIGHTGRAY);
  delay(500);

  //finished setup
  drawProgress(100, 100, "Finished!", SYSTEM_BLACK, SYSTEM_WHITE, SYSTEM_GRAY, SYSTEM_LIGHTGRAY);
  delay(500);
}

//updates the system app
void updateSystemApp() {
  if(millis() - lastUpdate > 1000) {
    uint8_t days = millis() / 86400000;
    uint8_t hours = (millis() - days * 86400000) / 3600000;
    uint8_t minutes = (millis() - days * 86400000 - hours * 3600000) / 60000;
    uint8_t seconds = (millis() - days * 86400000 - hours * 3600000 - minutes * 60000) / 1000;
    char uptime[15];
    sprintf(uptime, "%dd %02d:%02d:%02d", days, hours, minutes, seconds);
    gfx.fillBuffer(SYSTEM_BLACK);

    gfx.setColor(SYSTEM_WHITE);
    gfx.setFont(ArialRoundedMTBold_16);
    gfx.setTextAlignment(TEXT_ALIGN_CENTER);
    gfx.drawString(120, 20, "System Information");
    
    gfx.setFont(ArialRoundedMTBold_14);
    drawLabelValue(1, "Chip ID:", String(ESP.getChipId()));
    drawLabelValue(2, "Core Version:", String(ESP.getCoreVersion()));
    drawLabelValue(3, "CPU Frequency:", String(ESP.getCpuFreqMHz()) + "MHz");
    drawLabelValue(4, "Voltage:", String(ESP.getVcc() / 1024.0 + VOLTAGE_GAUGE, 2) + "V");
    drawLabelValue(5, "WiFi Strength:", String(WiFi.RSSI()) + "dBm");
    drawLabelValue(6, "Heap Memory:", String(ESP.getFreeHeap() / 1024.0, 2) + "kB");
    drawLabelValue(7, "Flash Chip ID:", String(ESP.getFlashChipId()));
    drawLabelValue(8, "Flash Memory:", String(ESP.getFlashChipRealSize() / 1024.0 / 1024.0, 2) + "MB");
    drawLabelValue(9, "Flash Chip Speed:", String(ESP.getFlashChipSpeed() / 1024.0 / 1024.0, 2) + "MHz");
    drawLabelValue(10, "SDK Version:", String(ESP.getSdkVersion()).substring(0, 5));
    drawLabelValue(11, "Sketch Size:", String(ESP.getSketchSize()) + "B");
    drawLabelValue(12, "Free Space:", String(ESP.getFreeSketchSpace()) + "B");
    drawLabelValue(13, "Uptime:", String(uptime));
    gfx.commit(0, 18);
    lastUpdate = millis();
  }
}

//draws current label text and value
void drawLabelValue(uint8_t line, String label, String value) {
  const uint8_t labelX = 15;
  const uint8_t valueX = 150;
  gfx.setTextAlignment(TEXT_ALIGN_LEFT);
  gfx.setColor(SYSTEM_WHITE);
  gfx.drawString(labelX, 42 + line * 15, label);
  gfx.setColor(SYSTEM_LIGHTGRAY);
  gfx.drawString(valueX, 42 + line * 15, value);
}

//handles touch actions for system app
void touchedSystemApp(TS_Point point) {}

//closes the system app
void closeAppScreen() {
  gfx.fillBuffer(SYSTEM_BLACK);
  gfx.commit(0, 0);
  gfx.fillBuffer(SYSTEM_BLACK);
  gfx.commit(0, 18);
}
