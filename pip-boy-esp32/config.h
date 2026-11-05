// User settings. Display pins live in the TFT_eSPI setup header
// (extras/TFT_eSPI/Setup_Waveshare35G_ESP32S3.h), see README.md.
#pragma once

// Touch controller (XPT2046), sharing the display's SPI bus
#define TOUCH_CS_PIN 2
#define TOUCH_IRQ_PIN 9

// POSIX timezone string: Belgrade (CET, UTC+1; CEST, UTC+2 during daylight saving)
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_SERVER "pool.ntp.org"

// Raw touch range; adjust using the values printed to Serial with TOUCH_DEBUG
#define TOUCH_X_MIN 200
#define TOUCH_X_MAX 3800
#define TOUCH_Y_MIN 200
#define TOUCH_Y_MAX 3800
#define TOUCH_FLIP_X false
#define TOUCH_FLIP_Y true
// Set to 1 to log each tap for calibration. Keep it 0 otherwise: tap positions reveal typed keys.
#define TOUCH_DEBUG 0

#define WIFI_CONNECT_TIMEOUT_MS 15000
