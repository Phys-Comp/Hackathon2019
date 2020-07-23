#include <ESP8266mDNS.h>
#include <WiFiClientSecure.h>
#include "SpotifyIcons.h"

#define BITS_PER_PIXEL 2
#define SPOTIFY_WIDTH 240
#define SPOTIFY_HEIGHT 160

#define SPOTIFY_BLACK 0
#define SPOTIFY_WHITE 1
#define SPOTIFY_GREEN 2
#define SPOTIFY_GRAY 3

const char* host = "api.spotify.com";
const int httpsPort = 443;
String espName = "esp8266";

uint16_t counter = 0;
String currentImageUrl = "";
boolean isDownloadingCover = false;
uint32_t lastDrawingUpdate = 0;
int lastLoop = 0;
long lastUpdate = 0;
uint16_t spotifyPalette[] = {ILI9341_BLACK,  // 0 -> RGB888: #000000
                             ILI9341_WHITE,  // 1 -> RGB888: #FFFFFF
                             0x368B,         // 2 -> RGB888: #32D25A
                             0x840F          // 3 -> RGB888: #808078
};

SpotifyClient client(clientId, clientSecret, "http://" + espName + "/callback/");
SpotifyData data;
SpotifyAuth auth;
SpotifyCallback spotifyCallback = &callback;

//opens and loads the spotify app
void openSpotify() {
  //drawing title bar with close button
  gfx = MiniGrafx(&display, BITS_PER_PIXEL, spotifyPalette, SPOTIFY_WIDTH, SPOTIFY_HEIGHT);
  gfx.init();
  gfx.fillBuffer(SPOTIFY_BLACK);
  gfx.commit(0, 0);
  gfx.fillBuffer(SPOTIFY_BLACK);
  gfx.commit(0,160);
  drawTitleBar("Spotify App", SPOTIFY_GREEN, SPOTIFY_WHITE, closeButton);
  
  //loading display
  drawProgress(10, 100, "Loading display...", SPOTIFY_BLACK, SPOTIFY_WHITE, SPOTIFY_GRAY, SPOTIFY_GREEN);
  delay(500);
  
  //loading file system
  drawProgress(30, 100, "Loading file system...", SPOTIFY_BLACK, SPOTIFY_WHITE, SPOTIFY_GRAY, SPOTIFY_GREEN);
  if (!SPIFFS.begin()) {
    drawProgress(40, 1000, "Formatting file system...", SPOTIFY_BLACK, SPOTIFY_WHITE, SPOTIFY_GRAY, SPOTIFY_GREEN);
    SPIFFS.format();
    SPIFFS.begin();
  }
  //setting up MDNS responder
  drawProgress(50, 100, "Setting up MDNS responder...", SPOTIFY_BLACK, SPOTIFY_WHITE, SPOTIFY_GRAY, SPOTIFY_GREEN);
  if (!MDNS.begin(espName.c_str())) {
    drawInfo("Error on setting up MDNS responder!", SPOTIFY_BLACK, SPOTIFY_GREEN);
  }
  espName = WiFi.localIP().toString();
  client.setRedirectUri("http://" + espName + "/callback/");
  
  //loading refresh token
  drawProgress(70, 100, "Loading refresh token...", SPOTIFY_BLACK, SPOTIFY_WHITE, SPOTIFY_GRAY, SPOTIFY_GREEN);
  String code = "";
  String grantType = "";
  String refreshToken = loadRefreshToken();
  if (refreshToken == "") {
    drawInfo("Authentication required.\nOpen browser at\nhttp://" + espName, SPOTIFY_BLACK, SPOTIFY_GREEN);    
    code = client.startConfigPortal();
    grantType = "authorization_code";
  }
  else {
    code = refreshToken;
    grantType = "refresh_token";
  }
  client.getToken(&auth, grantType, code);
  
  //saving refresh token
  drawProgress(90, 100, "Saving refresh token...", SPOTIFY_BLACK, SPOTIFY_WHITE, SPOTIFY_GRAY, SPOTIFY_GREEN);
  if (auth.refreshToken != "") {
    saveRefreshToken(auth.refreshToken);
  }
  //finished setup
  drawProgress(100, 100, "Finished!", SPOTIFY_BLACK, SPOTIFY_WHITE, SPOTIFY_GRAY, SPOTIFY_GREEN);
  client.setDrawingCallback(&spotifyCallback);
  delay(500);
  gfx.fillBuffer(SPOTIFY_BLACK);
  gfx.commit(0, 18);
}

//loads spotify refresh token from spiffs
String loadRefreshToken() {
  File file = SPIFFS.open("/refreshToken.txt", "r");
  if (!file) {
    drawInfo("Error on opening config file!", SPOTIFY_BLACK, SPOTIFY_GREEN);
    return "";
  }
  while(file.available()) {
      String token = file.readStringUntil('\r');
      file.close();
      return token;
  }
  return "";
}

//saves spotify refresh token in spiffs
void saveRefreshToken(String refreshToken) {
  File file = SPIFFS.open("/refreshToken.txt", "w+");
  if (!file) {
    drawInfo("Error on opening config file!", SPOTIFY_BLACK, SPOTIFY_GREEN);
    return;
  }
  file.println(refreshToken);
  file.close();
}

//updates the spotify data
void updateSpotify() {
  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    uint16_t responseCode = client.update(&data, &auth);

    //invalid refresh token
    if (responseCode == 401) {
      client.getToken(&auth, "refresh_token", auth.refreshToken);
      if (auth.refreshToken != "")
        saveRefreshToken(auth.refreshToken);
    }
    //authentification succeed
    if (responseCode == 200) {
      String selectedImageHref = data.image300Href;
      selectedImageHref.replace("https", "http");     

      //loading cover image
      if (selectedImageHref != currentImageUrl) {
        isDownloadingCover = true;
        client.downloadFile(selectedImageHref, "/cover.jpg");
        isDownloadingCover = false;
        currentImageUrl = selectedImageHref;
        drawJPEGFromSpiffs("/cover.jpg", 45, 18, 2, &callback);
        gfx.setColor(SPOTIFY_WHITE);
      }
    }
    //authentification failed (wrong Client-ID or Client-Secret)
    if (responseCode == 400) {
      drawInfo("Please define\nClient-ID and Client-Secret!", SPOTIFY_BLACK, SPOTIFY_GREEN);
    }
  }
  callback();
}

//spotify drawing callback
void callback() {
  drawSongInfo(&data);
}

//handles touch actions for spotify
void touchedSpotify(TS_Point point) {  
  uint16_t responseCode;

  //touched close button
  if(196 < point.x && point.x <= 240 && 0 < point.x && point.y <= 30) {
    Serial.println("[STMPE610] Touched close button.");
    closeSpotify();
  }
  //touched play/pause button
  if(88 < point.x && point.x <= 152 && 250 < point.y && point.y <= 315) {
    Serial.println("[STMPE610] Touched 'play'/'pause' button.");
    responseCode = client.playerCommand(&auth, data.isPlaying ? PlayerCmd::PAUSE : PlayerCmd::START);
    data.isPlaying = !data.isPlaying;
  }
  //touched previous button
  else if(40 < point.x && point.x <= 88 && 250 < point.y && point.y <= 315) {
    Serial.println("[STMPE610] Touched 'previous' button.");
    responseCode = client.playerCommand(&auth, PlayerCmd::PREV);
  }
  //touched next button
  else if(152 < point.x && point.x <= 200 && 250 < point.y && point.y <= 315) {
    Serial.println("[STMPE610] Touched 'next' button.");
    responseCode = client.playerCommand(&auth, PlayerCmd::NEXT);
  }
  //touched shuffle button
  else if(0 < point.x && point.x <= 40 && 250 < point.y && point.y <= 315) {
    Serial.println("[STMPE610] Touched shuffle button.");
    if(data.isShuffleState)
      responseCode = client.setShuffle(&auth, false);
    else
      responseCode = client.setShuffle(&auth, true);
  }
  //touched repeat button
  else if(200 < point.x && point.x <= 240 && 250 < point.y && point.y <= 315) {
    Serial.println("[STMPE610] Touched repeat button.");
    if(data.repeatMode == RepeatMode::OFF) {
      responseCode = client.setRepeatMode(&auth, RepeatMode::CONTEXT);
      data.repeatMode = RepeatMode::CONTEXT;
    } else if(data.repeatMode == RepeatMode::CONTEXT) {
      responseCode = client.setRepeatMode(&auth, RepeatMode::TRACK);
      data.repeatMode = RepeatMode::TRACK;
    } else {
      responseCode = client.setRepeatMode(&auth, RepeatMode::OFF);
      data.repeatMode = RepeatMode::OFF;
    }
  }
  //touched volume button
  else if(0 < point.x && point.x <= 44 && 160 < point.y && point.y <= 200) {
    Serial.println("[STMPE610] Touched volume button.");
    if(data.volumePercentage == 0) {
      responseCode = client.setVolume(&auth, 33);
    } else if(data.volumePercentage <= 33) {
      responseCode = client.setVolume(&auth, 66);
    } else if(data.volumePercentage <= 66) {
      responseCode = client.setVolume(&auth, 100);
    } else {
      responseCode = client.setVolume(&auth, 0);
    }
  }
  //touched progress bar
  else if(10 < point.x && point.x <= 230 && 200 < point.y && point.y <= 240) {
    Serial.println("[STMPE610] Touched progress bar.");
    int progressPercentage = (point.y - 10.0) / 220 * 100;
    responseCode = client.setPosition(&auth, &data, progressPercentage);
    data.progressMs = data.durationMs * progressPercentage / 100;
  }
  Serial.println("Command response: " + String(responseCode) + "\n");
}

//draws current spotify song info
void drawSongInfo(SpotifyData *data) {
  if (millis() - lastDrawingUpdate < 334) {
    return;
  }
  lastDrawingUpdate = millis();

  //update song progress
  long timeSinceUpdate = 0;
  if (data->isPlaying) {
    timeSinceUpdate = millis() - client.lastUpdate;
  }
  uint32_t progressMs = min(data->progressMs + timeSinceUpdate, data->durationMs);
  //update downloading state
  if (isDownloadingCover) {
    drawLoading(80, &counter, SPOTIFY_BLACK, SPOTIFY_WHITE);
  }
  counter++;

  if (data->isPlayerActive) {
    //drawing song progress
    uint8_t percentage = 100.0 * progressMs / data->durationMs;
    gfx.fillBuffer(SPOTIFY_BLACK);
    gfx.setColor(SPOTIFY_WHITE);
    gfx.setTextAlignment(TEXT_ALIGN_LEFT);
    gfx.drawString(10, 62, formatTime(progressMs));
    gfx.setTextAlignment(TEXT_ALIGN_RIGHT);
    gfx.drawString(230, 62, formatTime(data->durationMs));
    uint16_t progressX = 10 + 220 * percentage / 100;
    uint16_t progressY = 57;
    gfx.setColor(SPOTIFY_GRAY);
    gfx.drawLine(progressX, progressY, 230, progressY);
    gfx.setColor(SPOTIFY_WHITE);
    gfx.drawLine(10, progressY, progressX, progressY);
    gfx.fillCircle(progressX, progressY, 5);

    //drawing song title and artist
    String animatedTitle = data->title;
    uint8_t maxChar = 19;
    uint8_t excessChars = data->title.length() - maxChar;
    uint8_t currentPos = counter % excessChars;
    if (data->title.length() > maxChar) {
      animatedTitle = data->title.substring(currentPos, currentPos + maxChar + 1);
    }
    gfx.setTextAlignment(TEXT_ALIGN_CENTER);
    gfx.drawString(120, 7, animatedTitle);
    gfx.setColor(SPOTIFY_GRAY);
    gfx.drawString(120, 27, data->artistName);

    //drawing spotify icons/buttons
    gfx.drawPalettedBitmapFromPgm(88, 82, data->isPlaying ?  miniPause : miniPlay);
    gfx.drawPalettedBitmapFromPgm(53, 82, miniPrevious);
    gfx.drawPalettedBitmapFromPgm(168, 82, miniNext);
    gfx.drawPalettedBitmapFromPgm(210, 8, settings);
    gfx.drawPalettedBitmapFromPgm(20, 106, data->isShuffleState ? shuffleOn : shuffleOff);
    //drawing repeat mode
    if(data->repeatMode == RepeatMode::OFF) {
      gfx.drawPalettedBitmapFromPgm(206, 105, repeatOff);
    } else if(data->repeatMode == RepeatMode::CONTEXT) {
      gfx.drawPalettedBitmapFromPgm(206, 105, repeatContext);
    } else {
      gfx.drawPalettedBitmapFromPgm(206, 105, repeatTrack);
    }
    //drawing volume state
    if(data->volumePercentage == 0) {
      gfx.drawPalettedBitmapFromPgm(12, 9, volume0);
    } else if (data->volumePercentage <= 33) {
      gfx.drawPalettedBitmapFromPgm(12, 9, volume1);
    } else if (data->volumePercentage <= 66) {
      gfx.drawPalettedBitmapFromPgm(12, 9, volume2);
    } else {
      gfx.drawPalettedBitmapFromPgm(12, 9, volume3);
    }
    gfx.commit(0, 168);
  }
  //user offline
  else {
    gfx.fillBuffer(SPOTIFY_BLACK);
    gfx.setColor(SPOTIFY_WHITE);
    gfx.setTextAlignment(TEXT_ALIGN_CENTER);
    gfx.drawString(120, 12, "User is offline.");
    gfx.commit(0,168);
  }
}

//gets formatted time string
String formatTime(uint32_t time) {
  char time_str[10];
  uint8_t minutes = time / (1000 * 60);
  uint8_t seconds = (time / 1000) % 60;
  sprintf(time_str, "%2d:%02d", minutes, seconds);
  return String(time_str);
}

//closes the spotify app
void closeSpotify() {
  gfx.fillBuffer(SPOTIFY_BLACK);
  gfx.commit(0, 0);
  gfx.fillBuffer(SPOTIFY_BLACK);
  gfx.commit(0,160);
}
