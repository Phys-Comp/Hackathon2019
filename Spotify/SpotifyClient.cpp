/**The MIT License (MIT)

Copyright (c) 2018 by ThingPulse Ltd., https://thingpulse.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "SpotifyClient.h"

#define min(X, Y) (((X)<(Y))?(X):(Y))

SpotifyClient::SpotifyClient(String clientId, String clientSecret, String redirectUri) {
  this->clientId = clientId;
  this->clientSecret = clientSecret;
  this->redirectUri = redirectUri;
}

uint16_t SpotifyClient::update(SpotifyData *data, SpotifyAuth *auth) {
  this->data = data;
  request(auth, "GET", "player", "");
  uint16_t httpCode = request(auth, "GET", "player/currently-playing", "");
  lastUpdate = millis();
  if (httpCode == 200) {
    this->data->isPlayerActive = true;
  } else if (httpCode == 204) {
    this->data->isPlayerActive = false;
  }
  this->data = nullptr;
  return httpCode;
}

uint16_t SpotifyClient::playerCommand(SpotifyAuth *auth, PlayerCmd cmd) {
  return request(auth, getMethod(cmd), "player/" + getCommand(cmd), "");
}

uint16_t SpotifyClient::setVolume(SpotifyAuth *auth, int volumePercentage) {
  return request(auth, "PUT", "player/volume", String(volumePercentage));
}

uint16_t SpotifyClient::setShuffle(SpotifyAuth *auth, boolean shuffle) {
  return request(auth, "PUT", "player/shuffle", (shuffle ? "true" : "false"));
}

uint16_t SpotifyClient::setRepeatMode(SpotifyAuth *auth, RepeatMode mode) {
  return request(auth, "PUT", "player/repeat", getRepeatMode(mode));
}

uint16_t SpotifyClient::setPosition(SpotifyAuth *auth, SpotifyData *data, int progessPercentage) {
  uint32_t progressMs = (progessPercentage / 100.0) * data->durationMs;
  return request(auth, "PUT", "player/seek", String(int(progressMs)));
}

void SpotifyClient::getToken(SpotifyAuth *auth, String grantType, String code) {
  this->auth = auth;  //https://accounts.spotify.com/api/token
  isDataCall = false;
  JsonStreamingParser parser;
  parser.setListener(this);
  WiFiClientSecure client;
  const char* host = "accounts.spotify.com";
  const int port = 443;
  String url = "/api/token";
  int returnCode = client.connect(host, port);
  if (!returnCode) {
    Serial.println("Connection to " + String(host) + " failed! - Code: " + String(returnCode));
    return;
  }

  debug("\nRequesting URL: ");
  String codeParam = "code";
  if (grantType == "refresh_token") {
    codeParam = "refresh_token"; 
  }
  String authorizationRaw = clientId + ":" + clientSecret;
  String authorization = base64::encode(authorizationRaw, false);

  String content = "grant_type=" + grantType + "&" + codeParam + "=" + code + "&redirect_uri=" + redirectUri;
  String request = String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Authorization: Basic " + authorization + "\r\n" +
               "Content-Length: " + String(content.length()) + "\r\n" + 
               "Content-Type: application/x-www-form-urlencoded\r\n" + 
               "Connection: close\r\n\r\n" + 
               content;
  debug(request + "/n");
  client.print(request);  // This will send the request to the server
  
  int retryCounter = 0;
  while(!client.available()) {
    executeCallback();
    retryCounter++;
    if (retryCounter > 100) {
      Serial.println("Failed to get tokens!");
      return;
    }
    delay(10);
  }
  boolean isBody = false;
  char c;

  int size = 0;
  client.setNoDelay(false);
  while(client.connected() || client.available()) {
    while((size = client.available()) > 0) {
      String answer = client.readStringUntil('\r');
      for(int i = 0; i < answer.length(); i++) {
        c = answer.charAt(i);
        if (c == '{' || c == '[') {
          isBody = true;
        }
        if (isBody) {
          parser.parse(c);
          debug(String(c));
        } else {
          debug(String(c));
        }
      }
    }
    executeCallback();
  }
  this->data = nullptr;
}

void SpotifyClient::downloadFile(String url, String filename) {
  debug("Downloading " + url + " and saving as " + filename + ".\n");    
  HTTPClient http;
  debug("[HTTP] begin...\n");
  http.begin(url);  // configure server and url

  debug("[HTTP] GET...\n");
  int httpCode = http.GET();  // start connection and send HTTP header
  if(httpCode > 0) {
    fs::File f = SPIFFS.open(filename, "w+");
    if (!f) {
        Serial.println("File open failed!");
        return;
    }
    debug("[HTTP] GET... code: " + String(httpCode) + "\n");
    if(httpCode == HTTP_CODE_OK) {
      int total = http.getSize();  // get lenght of document (is -1 when Server sends no Content-Length header)
      int len = total;
      uint8_t buff[128] = { 0 };  // create buffer for read
      WiFiClient * stream = http.getStreamPtr();  // get tcp stream

      while(http.connected() && (len > 0 || len == -1)) {  // read all data from server
        size_t size = stream->available();  // get available data size

        if(size) {
          int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));  // read up to 128 byte
          f.write(buff, c);  // write it to Serial

          if(len > 0) {
              len -= c;
          }
          executeCallback();
        }
        delay(1);
      }
      debug("[HTTP] Connection closed or file end.ln\n");
    }
      f.close();
  } else {
      debug("[HTTP] GET... failed, error: " + String(http.errorToString(httpCode)) + "\n");
  }
  http.end();  
}

String SpotifyClient::startConfigPortal() {
  String oneWayCode = "";
  
  server.on ( "/", [this]() {
    debug("ClientID: " + this->clientId + "\n");
    debug("RedirectUri: " + this->redirectUri + "\n");
    server.sendHeader("Location", String("https://accounts.spotify.com/authorize/?client_id=" 
      + this->clientId 
      + "&response_type=code&redirect_uri=" 
      + this->redirectUri 
      + "&scope=user-read-private%20user-read-currently-playing%20user-read-playback-state%20user-modify-playback-state"), true);
    server.send ( 302, "text/plain", "");
  } );

  server.on ( "/callback/", [this, &oneWayCode](){
    if(!server.hasArg("code")) {server.send(500, "text/plain", "BAD ARGS"); return;}
    
    oneWayCode = server.arg("code");
    debug("Code: " + oneWayCode + "\n");  
    String message = "<html><head></head><body>Succesfully authenticated this device with Spotify. Restart your device now.</body></html>";
  
    server.send ( 200, "text/html", message );
  } );
  server.begin();

  if (WiFi.status() != WL_CONNECTED) {
	  Serial.println("WiFi not connected!");
  }
  debug("HTTP server started.\n");

  while(oneWayCode == "") {
    server.handleClient();
    yield();
  }
  server.stop();
  return oneWayCode;
}

uint16_t SpotifyClient::request(SpotifyAuth *auth, String method, String command, String content) {
  level = 0;
  isDataCall = true;
  currentParent = "";
  WiFiClientSecure client = WiFiClientSecure();
  JsonStreamingParser parser;
  parser.setListener(this);

  const char *host = "api.spotify.com";
  const int port = 443;
  String url = "/v1/me/" + command;
  int returnCode = client.connect(host, port);
  if (!returnCode) {
    Serial.println("Connection to " + String(host) + " failed! - Code: " + String(returnCode));
    return 0;
  }

  debug("\nRequesting URL: ");
  String request = method + " " + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Authorization: Bearer " + auth->accessToken + "\r\n";
  if(content == "") {
    request = request + "Content-Length: 0\r\n" + 
                        "Connection: close\r\n\r\n";
  } else {
    request = request + "Content-Type: application/json\r\n" +
                        "Content-Length: " + String(content.length()) + "\r\n" + 
                        "Connection: close\r\n\r\n" +
                        content; 
  }
  debug(request + "\n");
  client.print(request);  // This will send the request to the server
  
  int retryCounter = 0;
  while(!client.available()) {
    executeCallback();

    retryCounter++;
    if (retryCounter > 100) {
      return 0;
    }
    delay(10);
  }
  uint16_t bufLen = 1024;
  unsigned char buf[bufLen];
  boolean isBody = false;
  char c = ' ';

  int size = 0;
  client.setNoDelay(false);
  uint16_t httpCode = 0;
  while(client.connected() || client.available()) {
    while((size = client.available()) > 0) {
      if (isBody) {
        uint16_t len = min(bufLen, size);
        c = client.readBytes(buf, len);
        for (uint16_t i = 0; i < len; i++) {
          parser.parse(buf[i]);
        }
      } else {
        String line = client.readStringUntil('\r');
        debug(line + "\n");
        if (line.startsWith("HTTP/1.")) {
          httpCode = line.substring(9, line.indexOf(' ', 9)).toInt();
          debug("HTTP Code: " + String(httpCode) + "\n"); 
        }
        if (line == "\r" || line == "\n" || line == "") {
          isBody = true;
        }
      }
    }
    executeCallback();
  }
  return httpCode;
}

String SpotifyClient::getRootPath() {
  String path = "";
  for (uint8_t i = 1; i <= level; i++) {
    String currentLevel = rootPath[i];
    if (currentLevel == "") {
      break;
    }
    if (i > 1) {
      path += ".";
    }
    path += currentLevel;
  }
  return path;
}

void SpotifyClient::executeCallback() {
  if (callback != nullptr) {
    (*this->callback)();
  }
}

void SpotifyClient::debug(String text) {
  if(debugMode)
    Serial.print(text);
}

void SpotifyClient::startDocument() {
  level = 0;
}

void SpotifyClient::whitespace(char c) {}

void SpotifyClient::key(String key) {
  currentKey = String(key);
  rootPath[level] = key;
}

void SpotifyClient::value(String value) {
  if (isDataCall) {    
    String rootPath = this->getRootPath();
    
    if (currentKey == "progress_ms") {
      data->progressMs = value.toInt();
    }
    if (currentKey == "duration_ms") {
      data->durationMs = value.toInt();
    }
    if (currentKey == "name") {
      data->title = value;
    }
    if(currentKey == "name" && rootPath == "item.album.name") {
      data->albumName = value;
    }
    if (currentKey == "is_playing") {
      data->isPlaying = (value == "true" ? true : false);
    }
    if(currentKey == "volume_percent") {
      data->volumePercentage = value.toInt();
    }
    if(currentKey == "shuffle_state") {
      data->isShuffleState = (value == "true" ? true : false);
    }
    if(currentKey == "repeat_state") {
      if(value == "off")
        data->repeatMode = RepeatMode::OFF;
      else if(value == "track")
        data->repeatMode = RepeatMode::TRACK;
      else
        data->repeatMode = RepeatMode::CONTEXT;
    }
    if (currentKey == "height") {
      currentImageHeight = value.toInt();
    }
    if (currentKey == "url") {
      debug("HREF: " + rootPath + " = " + value + "\n");

      if (rootPath == "item.album.images.url") {
        if (currentImageHeight == 640) {
          data->image640Href = value;
        }
        if (currentImageHeight > 250 && currentImageHeight < 350) {
          data->image300Href = value;
        }
        if (currentImageHeight == 64) {
          data->image64Href = value;
        }
      }
    }
    if (rootPath == "item.album.artists.name") {
      data->artistName = value;
    }
    
  } else {
    debug(currentKey + " = " + value + "\n");
    if (currentKey == "access_token") {
      auth->accessToken = value;
    }
    if (currentKey == "token_type") {
      auth->tokenType = value;
    }
    if (currentKey == "expires_in") {
      auth->expiresIn = value.toInt();
    }
    if (currentKey == "refresh_token") {
      auth->refreshToken = value;
    }
    if (currentKey == "scope") {
      auth->scope = value;
    }
  }
}

void SpotifyClient::startObject() {
  currentParent = currentKey;
  level++;
}

void SpotifyClient::endObject() {
  level--;
  currentParent = "";
}

void SpotifyClient::startArray() {}

void SpotifyClient::endArray() {}

void SpotifyClient::endDocument() {}
