#include <Adafruit_VS1053.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
 
#define VS1053_RESET   -1
#define VS1053_CS      0
#define VS1053_DCS     5
#define VS1053_DREQ    2

Adafruit_VS1053 radioPlayer =  Adafruit_VS1053(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ);
WiFiClient client;
 
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
int httpPort = 80;

struct RadioChannel {
  String name;
  const char* host;
  const char* path;
};
struct RadioChannel channels[10];
struct RadioChannel lastChannel;

uint8_t mp3Buffer[32];
boolean stopped;
boolean paused;

void setup() {
  Serial.begin(115200);
  
  Serial.print("\nConnecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected!");  
  Serial.println("IP address: " + WiFi.localIP().toString() + "\n");
  
  while(!radioPlayer.begin()) {
     Serial.println("[Error] Couldn't find VS1053!");
     delay(5000);
  }
  loadChannels();
  setVolume(80);
  
  startRadio(channels[1]);
}

void loop() {
  if(Serial.available()) {
    String input = Serial.readStringUntil('\n');
    
    if(input == "play") {
      startRadio(lastChannel);
    } else if(input == "stop") {
      stopRadio();
    } else if(input == "pause") {
      pauseRadio();
    } else if(input == "resume") {
      resumeRadio();
    } else if (input.startsWith("volume ")) {
      int volume = input.substring(input.lastIndexOf(" "), input.length()).toInt();
      if(volume != 0) {
        setVolume(volume);
      }
    } else {
      for(int i = 0; i < 10; i++) {
        if(input.equalsIgnoreCase(channels[i].name)) {
          stopRadio();
          startRadio(channels[i]);
        }
      }
    }
  }  
  if(!paused && !stopped && radioPlayer.readyForData()) {
    if (client.available() > 0) {
      uint8_t bytesRead = client.read(mp3Buffer, 32);
      radioPlayer.playData(mp3Buffer, bytesRead);
    }
  }
}

//Radio Channel URLs (may be outdated)
void loadChannels() {
  channels[0] = {"SWR1",            "swr-edge-2032-dus-lg-cdn.cast.addradio.de",              "/swr/swr1/bw/aac/96/stream.aac?ar-distributor=f0a0"};          //SWR1
  channels[1] = {"SWR3",            "swr-edge-2033-dus-lg-cdn.cast.addradio.de",              "/swr/swr3/live/aac/96/stream.aac?ar-distributor=f0a0"};        //SWR3
  channels[2] = {"SWR4",            "swr-edge-20b5-fra-lg-cdn.cast.addradio.de",              "/swr/swr4/bw/aac/96/stream.aac?ar-distributor=f0a0"};          //SWR4
  channels[3] = {"Antenne 1",       "stream.antenne1.de",                                     "/a1stg/livestream1.aac?usid=M-A-04-0-0"};                      //Antenne 1
  channels[4] = {"Antenne Bayern",  "s5-webradio.antenne.de",                                 "/antenne/stream/mp3?aw_0_1st.playerid=radio.de"};              //Antenne Bayern
  channels[5] = {"BigFM",           "streams.bigfm.de",                                       "/bigfm-deutschland-128-mp3?usid=0-0-H-M-V-06"};                //BigFM
  channels[6] = {"Energy",          "185.52.127.162",                                         "/de/33005/mp3_128.mp3?origine=radio.net"};                     //Energy
  channels[7] = {"107.7",           "dieneue1077-ais-edge-3104-fra-eco-cdn.cast.addradio.de", "/dieneue1077/simulcast/high/stream.mp3?ar-distributor=f0a0"};  //107.7
}

//starts the radio
void startRadio(RadioChannel channel) {
  stopped = false;
  lastChannel = channel;
  if(!client.connect(channel.host, httpPort)) {
    Serial.println("Failed to connect to " + String(channel.host));
    return;
  }
  Serial.println("Connected to \"" + String(channel.host) + "\".");
  Serial.print("Requesting URL: ");
  Serial.println(channel.path);
  
  client.print(String("GET ") + channel.path + " HTTP/1.1\r\n" +
            "Host: " + channel.host + "\r\n" +
            "Connection: close\r\n\r\n");
  
  Serial.println("Playing \"" + channel.name + "\".");
}

//stops the radio
void stopRadio() {
  stopped = true;
  client.stop();
  client.flush();
  radioPlayer.reset();
  Serial.println("Stopped Radio.");
}

//pauses the radio
void pauseRadio() {
  paused = true;
  Serial.println("Paused Radio.");
}

//resumes the radio
void resumeRadio() {
  paused = false;
  Serial.println("Resumed Radio.");
}

//sets the volume
void setVolume(int volume) {
  volume = (volume < 0) ? 0 : (volume > 100) ? 100 : volume;
  radioPlayer.setVolume((uint8_t) (100 - volume), (uint8_t) (100 - volume));
  Serial.println("Switched volume to " + String(volume));
}
