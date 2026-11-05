// Waveshare 3.5inch RPi LCD (G) on ESP32-S3
// Pins from the vendor example's pin_config.h

#define USER_SETUP_ID 900

#define ST7796_DRIVER
#define TFT_WIDTH  320
#define TFT_HEIGHT 480
#define TFT_INVERSION_ON

#define TFT_SCLK 12
#define TFT_MOSI 13
#define TFT_MISO 4    // Only the touch controller drives MISO
#define TFT_CS   14
#define TFT_DC   17
#define TFT_RST  18

#define TFT_BL   5
#define TFT_BACKLIGHT_ON HIGH

// SPI3 (HSPI) like the vendor example; the default FSPI port crashes on ESP32 core 3.x+
#define USE_HSPI_PORT
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  16000000

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT
