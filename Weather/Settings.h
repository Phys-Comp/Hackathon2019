#include <simpleDSTadjust.h>

// Display settings
#define TFT_DC 15
#define TFT_CS 0
#define TOUCH_CS 2

// Touchscreen calibration settings
#define TS_MIN_X 150
#define TS_MIN_Y 130
#define TS_MAX_X 3800
#define TS_MAX_Y 4000

// WiFi Settings
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// OpenWeatherMap Settings
String OPEN_WEATHER_MAP_APP_ID = "YOUR_OPEN_WEATHER_MAP_ID";
String OPEN_WEATHER_MAP_LOCATION_ID = "YOUR_LOCATION_ID";
String OPEN_WEATHER_MAP_LANGUAGE = "de";
String DISPLAYED_CITY_NAME = "YOUR_CITY_NAME";
const boolean IS_METRIC = true;

#define MAX_FORECASTS 12
#define UPDATE_INTERVAL 600000  //10 minutes

// Language settings
const String WDAY_NAMES[] = {"SO", "MO", "DI", "MI", "DO", "FR", "SA"};
const String MONTH_NAMES[] = {"JAN", "FEB", "MÄR", "APR", "MAI", "JUN", "JUL", "AUG", "SEP", "OKT", "NOV", "DEZ"};
const String SUN_MOON_TEXT[] = {"Sonne", "Auf", "Unter", "Mond", "Alter", "Hell"};
const String MOON_PHASES[] = {"Neumond", "Zunehmend", "1. Viertel", "Zunehmend", "Vollmond", "Abnehmend", "3. Viertel", "Abnehmend"};

// Time settings
#define UTC_OFFSET +1
struct dstRule StartRule = {"CEST", Last, Sun, Mar, 2, 3600}; // Central European Summer Time = UTC/GMT +2 hours
struct dstRule EndRule = {"CET", Last, Sun, Oct, 2, 0};       // Central European Time = UTC/GMT +1 hour
