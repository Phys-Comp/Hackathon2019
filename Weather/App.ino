#include <FS.h>
#include <JPEGDecoder.h>

//draws info text
void drawInfo(String text, uint16_t background, uint16_t textColor) {
  gfx.fillBuffer(background);
  gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  gfx.setFont(ArialMT_Plain_16);
  gfx.setColor(textColor);
  gfx.drawString(120, 50, text);
  gfx.commit(0, 18);
}

//draws jpeg-image from spiffs
void drawJPEGFromSpiffs(String filename, int xPos, int yPos, uint8_t zoomFactor, void (*callback)()) {
  char buffer[filename.length() + 1];
  filename.toCharArray(buffer, filename.length() + 1);
  
  JpegDec.decodeFile(buffer);
  uint16_t *pImg;
  uint16_t mcuWidth = JpegDec.MCUWidth;
  uint16_t mcuHeigth = JpegDec.MCUHeight;

  while(JpegDec.read()){ 
    pImg = JpegDec.pImage;
    int mcuX = (JpegDec.MCUx * mcuWidth) / zoomFactor + xPos;
    int mcuY = (JpegDec.MCUy * mcuHeigth) / zoomFactor + yPos;
    display.setAddrWindow(mcuX, mcuY, mcuX + (mcuWidth / zoomFactor) - 1, mcuY + (mcuHeigth / zoomFactor) - 1);
    
    for (uint8_t y = 0; y < mcuHeigth; y++) {
      for (uint8_t x = 0; x < mcuWidth; x++) {
        if (x % zoomFactor == 0 && y % zoomFactor == 0) {
          display.pushColor(*pImg);
        }
        *pImg++;
      }
    }
    (*callback)();
  }
}

//draws loading animation
void drawLoading(uint16_t yPos, uint16_t *counter, uint16_t background, uint16_t color) {
  gfx.fillBuffer(background);
  gfx.setColor(color);
  gfx.drawCircle(100, yPos, 5);
  gfx.drawCircle(120, yPos, 5);
  gfx.drawCircle(140, yPos, 5);
  gfx.fillCircle(100 + 20 * (*counter % 3), yPos, 5);
  gfx.commit(0,18);
}

//draws progress bar
void drawProgress(uint8_t percentage, uint16_t yPos, String text, uint16_t background, uint16_t textColor, uint16_t frameColor, uint16_t barColor) {
  gfx.fillBuffer(background);
  gfx.setColor(textColor);
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  gfx.drawString(120, yPos + 6, text);
  gfx.setColor(frameColor);
  gfx.drawRect(10, yPos + 28, 220, 15);
  gfx.setColor(barColor);
  gfx.fillRect(12, yPos + 30, 216 * percentage / 100, 11);
  gfx.commit(0, 18);
}

//draws app title bar
void drawTitleBar(String appName, uint16_t color, uint16_t textColor, const char* closeButton) { 
  gfx.setColor(color);
  gfx.fillRect(0, 0, 240, 18);
  gfx.setColor(textColor);
  gfx.setFont(ArialRoundedMTBold_16);
  gfx.setTextAlignment(TEXT_ALIGN_LEFT);
  gfx.drawString(5, 0, appName);
  gfx.drawPalettedBitmapFromPgm(196, 0, closeButton);
  gfx.commit(0, 0);
}
