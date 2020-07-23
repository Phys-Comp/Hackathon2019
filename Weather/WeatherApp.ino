#include <Astronomy.h>
#include <OpenWeatherMapCurrent.h>
#include <OpenWeatherMapForecast.h>
#include <SparkFunBME280.h>
#include <Wire.h>
#include "Moonphases.h"
#include "WeatherIcons.h"

#define BITS_PER_PIXEL 2
#define FRAME_COUNT 3
#define WEATHER_WIDTH 240
#define WEATHER_HEIGHT 302

#define NTP_MIN_VALID_EPOCH 1533081600
#define NTP_SERVERS "0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org"

#define WEATHER_BLACK 0
#define WEATHER_WHITE 1
#define WEATHER_YELLOW 2
#define WEATHER_BLUE 3

ADC_MODE(ADC_VCC);
BME280 bme280;

OpenWeatherMapCurrentData currentWeather;
OpenWeatherMapForecastData forecasts[MAX_FORECASTS];
Astronomy::MoonData moonData;

simpleDSTadjust dstAdjusted(StartRule, EndRule);
time_t dstOffset = 0;

Carousel *carousel;
CarouselState state;
FrameCallback frames[] = { drawForecast1, drawForecast2, drawForecast3 };

uint16_t weatherPalette[] = {ILI9341_BLACK,   // 0 -> RGB888: #000000
                             ILI9341_WHITE,   // 1 -> RGB888: #FFFFFF
                             ILI9341_YELLOW,  // 2 -> RGB888: #FFFF00
                             0x7E3C           // 3 -> RGB888: #78C4E0
};

long carouselUpdate = 0;
long lastDownloadUpdate = 0;
long lastMessurement = 0;
uint8_t screen = 0;

//messured values
float temp = 0;
float humidity = 0;
float pressure = 0;
float altitude = 0;

//opens the weather app
void openWeatherApp() {
  gfx = MiniGrafx(&display, BITS_PER_PIXEL, weatherPalette, WEATHER_WIDTH, WEATHER_HEIGHT);
  gfx.init();
  gfx.fillBuffer(WEATHER_BLACK);
  gfx.commit();
  drawTitleBar("Weather App", WEATHER_BLUE, WEATHER_WHITE, closeButton);

  //loading display
  drawProgress(10, 100, "Loading display...", WEATHER_BLACK, WEATHER_YELLOW, WEATHER_WHITE, WEATHER_BLUE);
  delay(500);

  //initializing octopus sensors
  drawProgress(20, 100, "Initializing sensors...", WEATHER_BLACK, WEATHER_YELLOW, WEATHER_WHITE, WEATHER_BLUE);
  initOctopus();

  //loading file system
  drawProgress(30, 100, "Loading file system...", WEATHER_BLACK, WEATHER_YELLOW, WEATHER_WHITE, WEATHER_BLUE);
  if (!SPIFFS.begin()) {
    drawProgress(35, 1000, "Formatting file system...", WEATHER_BLACK, WEATHER_YELLOW, WEATHER_WHITE, WEATHER_BLUE);
    SPIFFS.format();
    SPIFFS.begin();
  }
  //setting up carousel
  drawProgress(40, 100, "Setting up carousel...", WEATHER_BLACK, WEATHER_YELLOW, WEATHER_WHITE, WEATHER_BLUE);
  carousel = new Carousel(&gfx, 0, 0, 240, 100);
  carousel->setFrames(frames, FRAME_COUNT);
  carousel->disableAllIndicators();

  updateData();
}

//initializes the octopus sensors
void initOctopus() {
  //initializing the I2C bus
  Wire.begin();
  if (Wire.status() != I2C_OK) {
    drawInfo("Error on starting I2C!", WEATHER_BLACK, WEATHER_BLUE);
    delay(2000);
  } else {
    //starting sensors
    bme280.begin();
  }
}

//updates the weather app
void updateWeatherApp() {
  gfx.fillBuffer(WEATHER_BLACK);
  if (screen == 0) {
    if (millis() > carouselUpdate) {
      carouselUpdate = millis() + carousel->update();
    }
    drawCurrentWeather();
    drawAstronomy();
  }
  else if (screen == 1) {
    drawCurrentWeatherDetail();
  }
  else if (screen == 2) {
    drawForecastTable(0);
  }
  else if (screen == 3) {
    drawForecastTable(4);
  }
  gfx.commit(0, 18);

  if (millis() - lastDownloadUpdate > UPDATE_INTERVAL) {
    updateData();
    lastDownloadUpdate = millis();
  }
}

//handles touch actions for weather app
void touchedWeatherApp(TS_Point point) {
  if (196 < point.x && point.x <= 240 && 0 < point.y && point.y <= 20) {
    closeWeatherApp();
  } else if (20 < point.y && point.y <= 320) {
    screen = (screen >= 3) ? 0 : screen + 1;
  }
}

//updates current weather data
void updateData() {
  //updating time
  drawProgress(50, 100, "Updating time...", WEATHER_BLACK, WEATHER_YELLOW, WEATHER_WHITE, WEATHER_BLUE);
  configTime(UTC_OFFSET * 3600, 0, NTP_SERVERS);
  time_t now;
  while ((now = time(nullptr)) < NTP_MIN_VALID_EPOCH) {
    Serial.print(".");
    delay(300);
  }
  dstOffset = UTC_OFFSET * 3600 + dstAdjusted.time(nullptr) - now;

  //updating conditions/settings
  drawProgress(70, 100, "Updating conditions...", WEATHER_BLACK, WEATHER_YELLOW, WEATHER_WHITE, WEATHER_BLUE);
  OpenWeatherMapCurrent *currentWeatherClient = new OpenWeatherMapCurrent();
  currentWeatherClient->setMetric(IS_METRIC);
  currentWeatherClient->setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  currentWeatherClient->updateCurrentById(&currentWeather, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);
  delete currentWeatherClient;
  currentWeatherClient = nullptr;

  //updating forecasts
  drawProgress(80, 100, "Updating forecasts...", WEATHER_BLACK, WEATHER_YELLOW, WEATHER_WHITE, WEATHER_BLUE);
  OpenWeatherMapForecast *forecastClient = new OpenWeatherMapForecast();
  forecastClient->setMetric(IS_METRIC);
  forecastClient->setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  uint8_t allowedHours[] = {12, 0};
  forecastClient->setAllowedHours(allowedHours, sizeof(allowedHours));
  forecastClient->updateForecastsById(forecasts, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID, MAX_FORECASTS);
  delete forecastClient;
  forecastClient = nullptr;

  //updating astronomy
  drawProgress(90, 100, "Updating astronomy...", WEATHER_BLACK, WEATHER_YELLOW, WEATHER_WHITE, WEATHER_BLUE);
  Astronomy *astronomy = new Astronomy();
  moonData = astronomy->calculateMoonData(now);
  moonData.phase = astronomy->calculateMoonPhase(now);
  delete astronomy;
  astronomy = nullptr;

  //finished update
  drawProgress(100, 100, "Finished!", WEATHER_BLACK, WEATHER_YELLOW, WEATHER_WHITE, WEATHER_BLUE);
  delay(500);
}

// draws current weather information
void drawCurrentWeather() {
  //drawing title
  gfx.setFont(ArialRoundedMTBold_16);
  gfx.setColor(WEATHER_YELLOW);
  gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  gfx.drawString(120, 12, "Aktuelles Wetter");

  //drawing weather icon
  gfx.setTransparentColor(WEATHER_BLACK);
  gfx.drawPalettedBitmapFromPgm(0, 37, getMeteoconIconFromProgmem(currentWeather.icon));

  //drawing city
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setColor(WEATHER_BLUE);
  gfx.setTextAlignment(TEXT_ALIGN_RIGHT);
  gfx.drawString(220, 45, DISPLAYED_CITY_NAME);

  //drawing temperature
  gfx.setFont(ArialRoundedMTBold_36);
  gfx.setColor(WEATHER_WHITE);
  gfx.setTextAlignment(TEXT_ALIGN_RIGHT);
  gfx.drawString(220, 60, String(currentWeather.temp, 1) + (IS_METRIC ? "°C" : "°F"));

  //drawing weather description
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setColor(WEATHER_YELLOW);
  gfx.setTextAlignment(TEXT_ALIGN_RIGHT);
  gfx.drawString(235, 100, currentWeather.description);
}

//draws first forecast
void drawForecast1(MiniGrafx *grafx, CarouselState* state, int16_t x, int16_t y) {
  drawForecastDetail(x + 10, y + 147, 0);
  drawForecastDetail(x + 95, y + 147, 1);
  drawForecastDetail(x + 180, y + 147, 2);
}

//draws second forecast
void drawForecast2(MiniGrafx *grafx, CarouselState* state, int16_t x, int16_t y) {
  drawForecastDetail(x + 10, y + 147, 3);
  drawForecastDetail(x + 95, y + 147, 4);
  drawForecastDetail(x + 180, y + 147, 5);
}

//draws third forecast
void drawForecast3(MiniGrafx *grafx, CarouselState* state, int16_t x, int16_t y) {
  drawForecastDetail(x + 10, y + 147, 6);
  drawForecastDetail(x + 95, y + 147, 7);
  drawForecastDetail(x + 180, y + 147, 8);
}

//draws forecast detail
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex) {
  if (forecasts[dayIndex].main == "") {
    return;
  }
  //drawing time/date
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  gfx.setColor(WEATHER_YELLOW);
  time_t time = forecasts[dayIndex].observationTime + dstOffset;
  struct tm *timeinfo = localtime(&time);
  gfx.drawString(x + 25, y - 15, WDAY_NAMES[timeinfo->tm_wday] + " " + String(timeinfo->tm_hour) + ":00");

  //drawing temperature
  gfx.setColor(WEATHER_WHITE);
  gfx.drawString(x + 25, y, String(forecasts[dayIndex].temp, 1) + (IS_METRIC ? "°C" : "°F"));

  //drawing icon
  gfx.drawPalettedBitmapFromPgm(x, y + 15, getMiniMeteoconIconFromProgmem(forecasts[dayIndex].icon));
  gfx.setColor(WEATHER_BLUE);
  gfx.drawString(x + 25, y + 60, String(forecasts[dayIndex].rain, 1) + (IS_METRIC ? "mm" : "in"));
}

//draws moonphase, sunrise/sunset and moonrise/moonset
void drawAstronomy() {
  gfx.setFont(MoonPhases_Regular_36);
  gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  gfx.setColor(WEATHER_WHITE);
  gfx.drawString(120, 257, String((char) (97 + (moonData.illumination * 26))));

  //drawing moonphase
  gfx.setColor(WEATHER_WHITE);
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  gfx.setColor(WEATHER_YELLOW);
  gfx.drawString(120, 232, MOON_PHASES[moonData.phase]);

  //drawing sunrise/sunset
  gfx.setTextAlignment(TEXT_ALIGN_LEFT);
  gfx.setColor(WEATHER_YELLOW);
  gfx.drawString(5, 232, SUN_MOON_TEXT[0]);
  gfx.setColor(WEATHER_WHITE);
  time_t time = currentWeather.sunrise + dstOffset;
  gfx.drawString(5, 258, getTime(&time));
  time = currentWeather.sunset + dstOffset;
  gfx.drawString(5, 273, getTime(&time));

  //drawing moonday and illumination percentage
  gfx.setTextAlignment(TEXT_ALIGN_RIGHT);
  gfx.setColor(WEATHER_YELLOW);
  gfx.drawString(235, 232, SUN_MOON_TEXT[3]);
  gfx.setColor(WEATHER_WHITE);
  float lunarMonth = 29.53;  //approximate moon age
  gfx.drawString(235, 258, String(moonData.phase <= 4 ? lunarMonth * moonData.illumination / 2.0 : lunarMonth - moonData.illumination * lunarMonth / 2.0, 1) + "d");
  gfx.drawString(235, 273, String(moonData.illumination * 100, 0) + "%");
}

//draws detailed current weather
void drawCurrentWeatherDetail() {
  String degreeSign = IS_METRIC ? "°C" : "°F";

  //getting indoor conditions
  if (millis() - lastMessurement > 10000) {
    temp = bme280.readTempC();
    humidity = bme280.readFloatHumidity();
    pressure = bme280.readFloatPressure() / 100.;
    altitude = bme280.readFloatAltitudeMeters();
    lastMessurement = millis();
  }
  gfx.setFont(ArialRoundedMTBold_16);
  gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  gfx.setColor(WEATHER_YELLOW);
  gfx.drawString(120, 12, "Wetterbedingungen");

  //drawing indoor conditions
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setColor(WEATHER_WHITE);
  gfx.drawString(120, 35, "Indoor:");
  drawLabelValue(1, "Temperatur:", String(temp, 2) + "°C");
  drawLabelValue(2, "Luftfeuchtigkeit:", String(humidity, 1) + "%");
  drawLabelValue(3, "Luftdruck:", String(pressure, 1) + "hPa");
  drawLabelValue(4, "Höhe über NN:", String(altitude, 2) + "m");

  //drawing outdoor weather conditions
  gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  gfx.setColor(WEATHER_WHITE);
  gfx.drawString(120, 125, "Outdoor:");
  drawLabelValue(7, "Temperatur:", String(currentWeather.temp, 2) + degreeSign);
  drawLabelValue(8, "Luftfeuchtigkeit:", String(currentWeather.humidity) + "%");
  drawLabelValue(9, "Luftdruck:", String(currentWeather.pressure) + "hPa");
  drawLabelValue(10, "Windgeschw.:", String(currentWeather.windSpeed, 2) + (IS_METRIC ? "m/s" : "mph") );
  drawLabelValue(11, "Windrichtung:", String(currentWeather.windDeg, 1) + "°");
  drawLabelValue(12, "Wolken:", String(currentWeather.clouds) + "%");
  drawLabelValue(13, "Sichtweite:", String(currentWeather.visibility) + "m");
  drawLabelValue(14, "Beschreibung:", "");
  gfx.setColor(WEATHER_WHITE);
  gfx.setTextAlignment(TEXT_ALIGN_LEFT);
  String firstChar = currentWeather.description.substring(0, 1);
  firstChar.toUpperCase();
  gfx.drawString(25, 267, firstChar + currentWeather.description.substring(1) + ".");
}

//draws current label text and value
void drawLabelValue(uint8_t line, String label, String value) {
  const uint8_t labelX = 15;
  const uint8_t valueX = 150;
  gfx.setTextAlignment(TEXT_ALIGN_LEFT);
  gfx.setColor(WEATHER_YELLOW);
  gfx.drawString(labelX, 42 + line * 15, label);
  gfx.setColor(WEATHER_WHITE);
  gfx.drawString(valueX, 42 + line * 15, value);
}

//draws forecast table
void drawForecastTable(uint8_t start) {
  gfx.setFont(ArialRoundedMTBold_14);
  uint16_t y = 0;
  String degreeSign = IS_METRIC ? "°C" : "°F";

  //iterating over forecast days
  for (uint8_t i = start; i < start + 4; i++) {
    gfx.setTextAlignment(TEXT_ALIGN_LEFT);
    y = 25 + (i - start) * 75;
    if (forecasts[i].main == "" || y > 320) {
      break;
    }
    //drawing time/date
    gfx.setColor(WEATHER_WHITE);
    gfx.setTextAlignment(TEXT_ALIGN_CENTER);
    time_t time = forecasts[i].observationTime + dstOffset;
    struct tm * timeinfo = localtime (&time);
    gfx.drawString(120, y - 15, WDAY_NAMES[timeinfo->tm_wday] + " " + String(timeinfo->tm_hour) + ":00");

    //drawing weather icon and short information
    gfx.drawPalettedBitmapFromPgm(0, y, getMiniMeteoconIconFromProgmem(forecasts[i].icon));
    gfx.setTextAlignment(TEXT_ALIGN_CENTER);
    gfx.setColor(WEATHER_YELLOW);
    gfx.setFont(ArialRoundedMTBold_14);
    gfx.drawString(25, y - 15, forecasts[i].main);

    //drawing temperature
    gfx.setTextAlignment(TEXT_ALIGN_LEFT);
    gfx.setColor(WEATHER_BLUE);
    gfx.drawString(55, y, "T:");
    gfx.setColor(WEATHER_WHITE);
    gfx.drawString(75, y, String(forecasts[i].temp, 1) + degreeSign);

    //drawing humidity
    gfx.setColor(WEATHER_BLUE);
    gfx.drawString(55, y + 15, "H:");
    gfx.setColor(WEATHER_WHITE);
    gfx.drawString(75, y + 15, String(forecasts[i].humidity) + "%");

    //drawing precipitation
    gfx.setColor(WEATHER_BLUE);
    gfx.drawString(55, y + 30, "P: ");
    gfx.setColor(WEATHER_WHITE);
    gfx.drawString(75, y + 30, String(forecasts[i].rain, 1) + (IS_METRIC ? "mm" : "in"));

    //drawing air pressure
    gfx.setColor(WEATHER_BLUE);
    gfx.drawString(135, y, "Pr:");
    gfx.setColor(WEATHER_WHITE);
    gfx.drawString(175, y, String(forecasts[i].pressure, 0) + "hPa");

    //drawing wind speed
    gfx.setColor(WEATHER_BLUE);
    gfx.drawString(135, y + 15, "WSp:");
    gfx.setColor(WEATHER_WHITE);
    gfx.drawString(175, y + 15, String(forecasts[i].windSpeed, 1) + (IS_METRIC ? "m/s" : "mph") );

    //drawing wind angle
    gfx.setColor(WEATHER_BLUE);
    gfx.drawString(135, y + 30, "WDi: ");
    gfx.setColor(WEATHER_WHITE);
    gfx.drawString(175, y + 30, String(forecasts[i].windDeg, 0) + "°");
  }
}

//gets formatted time
String getTime(time_t *timestamp) {
  struct tm *timeInfo = gmtime(timestamp);
  char buff[6];
  sprintf(buff, "%02d:%02d", timeInfo->tm_hour, timeInfo->tm_min);
  return String(buff);
}

//closes the weather app
void closeWeatherApp() {
  gfx.fillBuffer(WEATHER_BLACK);
  gfx.commit();
}
