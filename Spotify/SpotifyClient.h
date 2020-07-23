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

#pragma once
#include <Arduino.h>
#include <base64.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <JsonListener.h>
#include <JsonStreamingParser.h>

enum class PlayerCmd { PAUSE, START, NEXT, PREV };
static const String cmdStrings[] = {"pause", "play", "next", "previous"};
static const String cmdMethods[] = {"PUT", "PUT", "POST", "POST"};

enum class RepeatMode { OFF, TRACK, CONTEXT};
static const String repeatStates[] = {"off", "track", "context"};

typedef void (*SpotifyCallback)();

typedef struct SpotifyAuth {
  String accessToken;   // "access_token": "XXX"
  String tokenType;     // "token_type":"Bearer"
  uint16_t expiresIn;   // "expires_in":3600,
  String refreshToken;  // "refresh_token":"XX"
  String scope;         // "scope":"user-modify-playback-state", "user-read-playback-state", "user-read-currently-playing", "user-read-private"
} SpotifyAuth;

typedef struct SpotifyData {
  boolean isPlayerActive;
  String albumName;
  String title;
  String artistName;
  boolean isPlaying;
  uint32_t progressMs;
  uint32_t durationMs;
  uint32_t volumePercentage;
  boolean isShuffleState;
  RepeatMode repeatMode;
  String image640Href;
  String image300Href;
  String image64Href;
  
} SpotifyData;

class SpotifyClient: public JsonListener {
  private:
    SpotifyData *data;
    SpotifyAuth *auth;
    bool isDataCall;
    String currentKey;
    String currentParent;
    uint8_t level = 0;
    String rootPath[10];
    uint16_t currentImageHeight;
    SpotifyCallback *callback;
    String clientId;
    String clientSecret;
    String redirectUri;
    ESP8266WebServer server;
    boolean debugMode = true;

    uint16_t request(SpotifyAuth *auth, String method, String command, String content);
    boolean checkFavoriteTracks(SpotifyAuth * auth, String spotifyId);
    String getMethod(PlayerCmd playerCmd) { return cmdMethods[int(playerCmd)]; }
    String getCommand(PlayerCmd playerCmd) { return cmdStrings[int(playerCmd)]; }
    String getRootPath();
    void executeCallback();
    void debug(String text);

  public:
    uint32_t lastUpdate = 0;
    
    SpotifyClient(String clientId, String clientSecret, String redirectUri);
    void setRedirectUri(String redirectUri) { this->redirectUri = redirectUri; }
    uint16_t update(SpotifyData *data, SpotifyAuth *auth);
    void close();
    uint16_t playerCommand(SpotifyAuth *auth, PlayerCmd cmd);
    uint16_t setVolume(SpotifyAuth *auth, int volumePercentage);
    uint16_t setShuffle(SpotifyAuth *auth, boolean shuffle);
    uint16_t setRepeatMode(SpotifyAuth *auth, RepeatMode mode);
    uint16_t setPosition(SpotifyAuth *auth, SpotifyData *data, int progessPercentage);
    String getRepeatMode(RepeatMode mode) { return repeatStates[int(mode)]; }
    void getToken(SpotifyAuth *auth, String grantType, String code);
    void downloadFile(String url, String filename);
    void setDrawingCallback(SpotifyCallback *callback) { this->callback = callback; }
    String startConfigPortal();

    virtual void whitespace(char c);
    virtual void startDocument();
    virtual void key(String key);
    virtual void value(String value);
    virtual void endArray();
    virtual void endObject();
    virtual void endDocument();
    virtual void startArray();
    virtual void startObject(); 
};
