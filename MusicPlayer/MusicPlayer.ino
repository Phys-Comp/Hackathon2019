#include <Adafruit_VS1053.h>
#include <SD.h>
#include <SPI.h>

//#define VS1053_RESET  9     // VS1053 Reset pin
#define VS1053_CS       0     // VS1053 Chip Select pin
#define VS1053_DCS      5     // VS1053 Data/Command Select pin
#define VS1052_DREQ     2     // VS1053 Data Request
#define CARD_CS         15    // SD Card Chip Select pin

#define FILE_DIRECTORY "/"

Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(VS1053_CS, VS1053_DCS, VS1052_DREQ, CARD_CS);
File directory;
boolean stopped = false;

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- Music Player ---\n");

  while(!musicPlayer.begin()) {
    Serial.println("[ERROR] Couldn't find VS1053!");
    delay(10000);
  }
  Serial.println("VS1053 connected!");
  
  while(!SD.begin(CARD_CS)) {
    Serial.println("[ERROR] Failed to connect SD card!");
    delay(10000);
  }
  directory = SD.open(FILE_DIRECTORY);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);
  setVolume(80);
  
  playMusic();
}

void loop() {
  if(Serial.available()) {
    String input = Serial.readStringUntil('\n');
    
    if(input == "play") {
      playMusic();
    } else if(input == "pause") {
      pauseMusic();
    } else if(input == "resume") {
      resumeMusic();
    } else if(input == "stop") {
      stopMusic();
    } else if(input == "next" || input == "skip") {
      skipMusic();
    } else if (input.startsWith("volume ")) {
      int volume = input.substring(input.lastIndexOf(" "), input.length()).toInt();
      if(volume != 0) {
        setVolume(volume);
      }
    } else if(SD.exists(input) && musicPlayer.isMP3File(input.c_str())) {
      playSong(input.c_str());
    } else if(SD.exists(input + ".mp3") && musicPlayer.isMP3File((input + ".mp3").c_str())) {
      playSong((input + ".mp3").c_str());
    }
  }
  if(!stopped && musicPlayer.stopped()) {
    playMusic();
  }
  delay(50);
}

//starts playing the next song
void playMusic() {
  musicPlayer.stopPlaying();
  stopped = false;
  
  File entry = directory.openNextFile();
  while(!musicPlayer.isMP3File(entry.name())) {
    entry.close();
    entry = directory.openNextFile();
    
    if(!entry) {
      directory.seek(0);
    }
  }
  Serial.printf("Playing \"%s\".\n", entry.name());
  musicPlayer.startPlayingFile(entry.name());  //playing file in background
}

//plays a song
void playSong(const char* filepath) {
  if(SD.exists(filepath) && musicPlayer.isMP3File(filepath)) {
    musicPlayer.stopPlaying();
    stopped = false;
    
    Serial.printf("Playing \"%s\".\n", filepath);
    musicPlayer.startPlayingFile(filepath);  //playing file in background
    
    File entry = SD.open(filepath);
    if(entry) {
      directory.seek(musicPlayer.mp3_ID3Jumper(entry));
    }
  }
}

//pauses the song
void pauseMusic() {
  musicPlayer.pausePlaying(true);
  Serial.println("Paused.");
}

//resumes the song
void resumeMusic() {
  musicPlayer.pausePlaying(false);
  Serial.println("Resumed.");
}

//sets the volume of the player
void setVolume(int volume) {
  volume = (volume < 0) ? 0 : (volume > 100) ? 100 : volume;
  musicPlayer.setVolume((uint8_t) (100 - volume), (uint8_t) (100 - volume));
  Serial.println("Switched volume to " + volume);
}

//stops the song
void stopMusic() {
  stopped = true;
  musicPlayer.stopPlaying();
  Serial.println("Stopped.");
}

//skips the current song
void skipMusic() {
  musicPlayer.stopPlaying();
  playMusic();
  Serial.println("Skipped Song.");
}
