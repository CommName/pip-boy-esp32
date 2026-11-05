// Screen layout, types and constant tables
#pragma once

#include <stdint.h>

#define SCREEN_W 480
#define SCREEN_H 320

// The screen can't be read back, so everything is rendered from state into an
// off-screen strip and pushed. Drawing helpers take screen coordinates.
#define STRIP_H 40
#define SY(y) ((y) - strip_y)

// Rolling CRT refresh band
#define BAND_H 144
#define BAND_STEP 4
#define BAND_FRAME_MS 25
#define BAND_PAUSE_MS 2500

// Big clock digits: 5x7 block glyphs, each block CELL x CELL pixels
#define CELL 13
#define GLYPH_W (5 * CELL)
#define GLYPH_H (7 * CELL)
#define CLOCK_W (4 * GLYPH_W + 5 * CELL)
#define CLOCK_X ((SCREEN_W - CLOCK_W) / 2)
#define CLOCK_Y 120

#define HEADER_H 62
#define STATUS_Y 282
#define STATUS_H 26

const uint8_t DIGITS[10][7] = {
    {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}, // 0
    {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}, // 1
    {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111}, // 2
    {0b11111, 0b00010, 0b00100, 0b00010, 0b00001, 0b10001, 0b01110}, // 3
    {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010}, // 4
    {0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110}, // 5
    {0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110}, // 6
    {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000}, // 7
    {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110}, // 8
    {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100}, // 9
};

// Wi-Fi setup screens, shown at startup in place of the clock
#define SSID_MAX 32
#define PASS_MAX 63

struct Button
{
    int x, y, w, h;
};

enum SetupScreen
{
    SETUP_MESSAGE,
    SETUP_SCAN,
    SETUP_KEYBOARD
};

// Message screen
const Button BTN_LEFT = {40, 230, 190, 60};
const Button BTN_RIGHT = {250, 230, 190, 60};

// Scan list
#define SCAN_MAX 30
#define SCAN_ROWS 5
#define SCAN_Y 46
#define SCAN_PITCH 44
#define SCAN_ROW_H 40

struct ScanEntry
{
    char ssid[SSID_MAX + 1];
    int rssi;
    bool open;
};

const Button BTN_PREV = {10, 270, 100, 44};
const Button BTN_NEXT = {115, 270, 100, 44};
const Button BTN_RESCAN = {220, 270, 125, 44};
const Button BTN_OTHER = {350, 270, 120, 44};

// On-screen keyboard
#define KB_Y 68
#define KB_PITCH 50
#define KB_KEY_PITCH 48
#define KB_KEY_W 46
#define KB_KEY_H 46
#define KB_FIELD_Y 26
#define KB_FIELD_H 36
#define KB_LOWER 0
#define KB_UPPER 1
#define KB_SYMBOLS 2

const char *const KB_LAYERS[3][4] = {
    {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm"},
    {"1234567890", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"},
    {"!@#$%^&*()", "-_=+[]{}\\|", ";:'\",.<>/", "?`~"},
};
const int KB_ROW_X[4] = {1, 1, 25, 49};
const Button BTN_DEL = {385, KB_Y + 3 * KB_PITCH, 94, KB_KEY_H};
const Button BTN_SHIFT = {4, KB_Y + 4 * KB_PITCH, 72, KB_KEY_H};
const Button BTN_SYM = {80, KB_Y + 4 * KB_PITCH, 62, KB_KEY_H};
const Button BTN_SPACE = {146, KB_Y + 4 * KB_PITCH, 130, KB_KEY_H};
const Button BTN_CANCEL = {280, KB_Y + 4 * KB_PITCH, 100, KB_KEY_H};
const Button BTN_OK = {384, KB_Y + 4 * KB_PITCH, 92, KB_KEY_H};
const Button BTN_SHOW = {400, KB_FIELD_Y, 72, KB_FIELD_H};
