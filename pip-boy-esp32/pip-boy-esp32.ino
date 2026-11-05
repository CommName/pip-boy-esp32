// Pip-Boy style clock for Waveshare 3.5inch RPi LCD (G) on ESP32-S3
//
// User settings are in config.h, screen layout in ui.h. Display pins are set by
// extras/TFT_eSPI/Setup_Waveshare35G_ESP32S3.h, which must be installed into the
// TFT_eSPI library (see README.md).
//
// Code is split across the sketch's tabs:
//   clock.ino       clock state from NTP time
//   crt.ino         strip renderer, palette and CRT effects
//   screens.ino     drawing of the clock and setup screens
//   touch.ino       touch input
//   wifi_setup.ino  interactive Wi-Fi setup

#include <SPI.h>
#include <WiFi.h>
#include <time.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>

#include "config.h"
#include "ui.h"

TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen touch(TOUCH_CS_PIN, TOUCH_IRQ_PIN);

// Render strip
TFT_eSprite strip = TFT_eSprite(&tft);
int strip_y = 0; // Screen row of the strip's first line
int band_y = -BAND_H;

// Pip-Boy palette; each fill colour has a darker variant for the CRT scanlines
uint16_t COL_BG, COL_BG_SCAN;
uint16_t COL_FG, COL_FG_SCAN;
uint16_t COL_DIM;
uint16_t COL_BOX, COL_BOX_SCAN;

// Clock screen state
bool synced = false;
const char *message = "CONNECTING TO WI-FI...";
int digits[4] = {0, 0, 0, 0};
bool colon_on = true;
char status_date[24] = "";
int wifi_bars = 0;

// Wi-Fi setup state
bool setup_active = false;
SetupScreen setup_screen = SETUP_MESSAGE;

char cand_ssid[SSID_MAX + 1] = ""; // Network being connected to
char cand_pass[PASS_MAX + 1] = "";

// Message screen
const char *msg_title = "";
const char *msg_line1 = "";
const char *msg_line2 = "";
const char *msg_left = nullptr;
const char *msg_right = nullptr;

// Scan list
ScanEntry nets[SCAN_MAX];
int net_count = 0;
int scan_page = 0;

// On-screen keyboard
char kb_label[64] = "";
char kb_text[PASS_MAX + 1] = "";
int kb_max = 0;
int kb_layer = KB_LOWER;
bool kb_secret = false; // Secret text can be masked with the SHOW key
bool kb_masked = false;

void setup()
{
    Serial.begin(115200);

    tft.init();
    tft.setRotation(1); // Landscape, 480x320

    // Touch shares the display's SPI bus (SCK 12, MOSI 13, MISO 4)
    touch.begin(tft.getSPIinstance());
    touch.setRotation(1);

    initPalette();

    strip.setColorDepth(16);
    if (!strip.createSprite(SCREEN_W, STRIP_H))
        Serial.println("Not enough RAM for the render strip");

    runWifiSetup();
    configTzTime(TIMEZONE, NTP_SERVER);
}

void loop()
{
    updateClock();
    updateBand();

    delay(5);
}
