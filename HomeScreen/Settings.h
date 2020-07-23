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

// WiFi settings
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Time settings
#define UPDATE_INTERVAL 600000  //10 minutes
#define UTC_OFFSET +1
struct dstRule StartRule = {"CEST", Last, Sun, Mar, 2, 3600}; // Central European Summer Time = UTC/GMT +2 hours
struct dstRule EndRule = {"CET", Last, Sun, Oct, 2, 0};       // Central European Time = UTC/GMT +1 hour
