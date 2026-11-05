// Pip-Boy style clock for Waveshare 3.5inch RPi LCD (G) on ESP32-S3
// Display pins are configured in TFT_eSPI/User_Setups/Setup_Waveshare35G_ESP32S3.h

#define TOUCH_CS_PIN 2
#define TOUCH_IRQ_PIN 9

#include <SPI.h>
#include <WiFi.h>
#include <time.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>

// POSIX timezone string: Belgrade (CET, UTC+1; CEST, UTC+2 during daylight saving)
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_SERVER "pool.ntp.org"

// Raw touch range; adjust using the values printed to Serial
#define TOUCH_X_MIN 200
#define TOUCH_X_MAX 3800
#define TOUCH_Y_MIN 200
#define TOUCH_Y_MAX 3800
#define TOUCH_FLIP_X false
#define TOUCH_FLIP_Y true
// Logs each tap for calibration. Set to 0 afterwards: tap positions reveal typed keys.
#define TOUCH_DEBUG 1

#define SCREEN_W 480
#define SCREEN_H 320

TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen touch(TOUCH_CS_PIN, TOUCH_IRQ_PIN);

// The screen can't be read back, so everything is rendered from state into this
// off-screen strip and pushed. Drawing helpers take screen coordinates.
#define STRIP_H 40
TFT_eSprite strip = TFT_eSprite(&tft);
int strip_y = 0; // Screen row of the strip's first line
#define SY(y) ((y) - strip_y)

// Rolling CRT refresh band
#define BAND_H 144
#define BAND_STEP 4
#define BAND_FRAME_MS 25
#define BAND_PAUSE_MS 2500
int band_y = -BAND_H;

// Pip-Boy palette; each fill colour has a darker variant for the CRT scanlines
uint16_t COL_BG, COL_BG_SCAN;
uint16_t COL_FG, COL_FG_SCAN;
uint16_t COL_DIM;
uint16_t COL_BOX, COL_BOX_SCAN;

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

// Screen state
bool synced = false;
const char *message = "CONNECTING TO WI-FI...";
int digits[4] = {0, 0, 0, 0};
bool colon_on = true;
char status_date[24] = "";
int wifi_bars = 0;

// Wi-Fi setup screens, shown at startup in place of the clock
#define WIFI_CONNECT_TIMEOUT_MS 15000
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

ScanEntry nets[SCAN_MAX];
int net_count = 0;
int scan_page = 0;
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

char kb_label[64] = "";
char kb_text[PASS_MAX + 1] = "";
int kb_max = 0;
int kb_layer = KB_LOWER;
bool kb_secret = false; // Secret text can be masked with the SHOW key
bool kb_masked = false;

bool stripOverlaps(int y, int h)
{
    return y < strip_y + STRIP_H && y + h > strip_y;
}

// Fill a rectangle and darken every third screen row to imitate CRT scanlines
void crtRect(int x, int y, int w, int h, uint16_t color, uint16_t scan)
{
    int top = max(y, strip_y);
    int bottom = min(y + h, strip_y + STRIP_H);
    if (top >= bottom)
        return;
    strip.fillRect(x, SY(top), w, bottom - top, color);
    for (int row = top + (2 - top % 3); row < bottom; row += 3)
    {
        strip.drawFastHLine(x, SY(row), w, scan);
    }
}

void drawDigit(int x, int digit)
{
    for (int row = 0; row < 7; row++)
    {
        for (int col = 0; col < 5; col++)
        {
            if (DIGITS[digit][row] & (0b10000 >> col))
                crtRect(x + col * CELL, CLOCK_Y + row * CELL, CELL, CELL, COL_FG, COL_FG_SCAN);
        }
    }
}

void drawClockDigits()
{
    const int xs[4] = {CLOCK_X, CLOCK_X + GLYPH_W + CELL,
                       CLOCK_X + 2 * GLYPH_W + 4 * CELL, CLOCK_X + 3 * GLYPH_W + 5 * CELL};
    for (int i = 0; i < 4; i++)
    {
        drawDigit(xs[i], digits[i]);
    }

    if (colon_on)
    {
        int x = CLOCK_X + 2 * GLYPH_W + 2 * CELL;
        crtRect(x, CLOCK_Y + 2 * CELL, CELL, CELL, COL_FG, COL_FG_SCAN);
        crtRect(x, CLOCK_Y + 4 * CELL, CELL, CELL, COL_FG, COL_FG_SCAN);
    }
}

void drawHeader()
{
    const char *tabs[] = {"STAT", "INV", "DATA", "MAP", "RADIO"};
    const int centers[] = {100, 175, 250, 325, 405};
    const int active = 2;
    const int line_y = 36;

    strip.setFreeFont(&FreeMonoBold12pt7b);
    strip.setTextDatum(MC_DATUM);
    strip.setTextColor(COL_FG);
    for (int i = 0; i < 5; i++)
    {
        strip.drawString(tabs[i], centers[i], SY(22));
    }

    // Underline with a bracket raised around the active tab
    int x1 = centers[active] - 38, x2 = centers[active] + 38;
    strip.drawFastHLine(56, SY(line_y), x1 - 56, COL_FG);
    strip.drawFastVLine(x1, SY(10), line_y - 10, COL_FG);
    strip.drawFastHLine(x1, SY(10), 6, COL_FG);
    strip.drawFastVLine(x2, SY(10), line_y - 10, COL_FG);
    strip.drawFastHLine(x2 - 5, SY(10), 6, COL_FG);
    strip.drawFastHLine(x2, SY(line_y), 462 - x2, COL_FG);

    // Small icons in the corners
    strip.fillCircle(30, SY(22), 10, COL_FG);
    strip.fillCircle(30, SY(22), 4, COL_BG);
    strip.drawRect(444, SY(14), 22, 16, COL_FG);
    strip.fillRect(448, SY(18), 14, 8, COL_FG);

    // Sub-tabs, CLOCK selected
    strip.setFreeFont(&FreeMonoBold9pt7b);
    strip.setTextDatum(TL_DATUM);
    strip.drawString("CLOCK", x1 + 4, SY(44));
    strip.setTextColor(COL_DIM);
    strip.drawString("STATS", x1 + 68, SY(44));
    strip.drawString("MAINTENANCE", x1 + 128, SY(44));
}

int rssiToBars(int rssi)
{
    return rssi > -55 ? 4 : rssi > -65 ? 3
                        : rssi > -75   ? 2
                                       : 1;
}

void drawSignalBars(int x, int bottom, int bars)
{
    for (int i = 0; i < 4; i++)
    {
        int bh = 5 + i * 4;
        int bx = x + i * 12;
        if (i < bars)
            strip.fillRect(bx, SY(bottom - bh), 8, bh, COL_FG);
        else
            strip.drawRect(bx, SY(bottom - bh), 8, bh, COL_DIM);
    }
}

void drawStatusBar()
{
    const int y = STATUS_Y, h = STATUS_H;

    // Date and time
    crtRect(10, y, 240, h, COL_BOX, COL_BOX_SCAN);
    strip.setFreeFont(&FreeMonoBold9pt7b);
    strip.setTextDatum(ML_DATUM);
    strip.setTextColor(COL_FG);
    strip.drawString(status_date, 18, SY(y + h / 2));

    // Battery icon (decorative)
    crtRect(256, y, 84, h, COL_BOX, COL_BOX_SCAN);
    strip.drawRect(272, SY(y + 6), 46, 14, COL_FG);
    strip.fillRect(318, SY(y + 10), 4, 6, COL_FG);
    strip.fillRect(275, SY(y + 9), 36, 8, COL_FG);

    // Wi-Fi signal strength
    crtRect(346, y, 124, h, COL_BOX, COL_BOX_SCAN);
    strip.drawString("WIFI", 356, SY(y + h / 2));
    drawSignalBars(412, y + 21, wifi_bars);
}

void drawButton(const Button &b, const char *label, bool active)
{
    if (!stripOverlaps(b.y, b.h))
        return;
    if (active)
        crtRect(b.x, b.y, b.w, b.h, COL_FG, COL_FG_SCAN);
    else
        crtRect(b.x, b.y, b.w, b.h, COL_BOX, COL_BOX_SCAN);
    strip.drawRect(b.x, SY(b.y), b.w, b.h, COL_FG);
    strip.setFreeFont(strlen(label) == 1 ? &FreeMonoBold12pt7b : &FreeMonoBold9pt7b);
    strip.setTextDatum(MC_DATUM);
    strip.setTextColor(active ? COL_BG : COL_FG);
    strip.drawString(label, b.x + b.w / 2, SY(b.y + b.h / 2));
}

void drawSetupTitle(const char *title)
{
    if (!stripOverlaps(0, 40))
        return;
    strip.setFreeFont(&FreeMonoBold12pt7b);
    strip.setTextDatum(MC_DATUM);
    strip.setTextColor(COL_FG);
    strip.drawString(title, SCREEN_W / 2, SY(20));
    strip.drawFastHLine(10, SY(38), SCREEN_W - 20, COL_FG);
}

void drawCenteredLine(const char *text, int y)
{
    if (!stripOverlaps(y - 15, 30))
        return;
    strip.setFreeFont(&FreeMonoBold12pt7b);
    strip.setTextDatum(MC_DATUM);
    strip.setTextColor(COL_FG);
    strip.drawString(text, SCREEN_W / 2, SY(y));
}

void drawMessageScreen()
{
    drawSetupTitle(msg_title);
    drawCenteredLine(msg_line1, 110);
    drawCenteredLine(msg_line2, 160);
    if (msg_left)
        drawButton(BTN_LEFT, msg_left, false);
    if (msg_right)
        drawButton(BTN_RIGHT, msg_right, false);
}

int scanPageCount()
{
    return max(1, (net_count + SCAN_ROWS - 1) / SCAN_ROWS);
}

void drawScanScreen()
{
    drawSetupTitle("SELECT NETWORK");

    if (net_count == 0)
        drawCenteredLine("NO NETWORKS FOUND", 150);

    for (int i = 0; i < SCAN_ROWS; i++)
    {
        int index = scan_page * SCAN_ROWS + i;
        int y = SCAN_Y + i * SCAN_PITCH;
        if (index >= net_count)
            break;
        if (!stripOverlaps(y, SCAN_ROW_H))
            continue;

        crtRect(10, y, SCREEN_W - 20, SCAN_ROW_H, COL_BOX, COL_BOX_SCAN);
        strip.setFreeFont(&FreeMonoBold9pt7b);
        strip.setTextDatum(ML_DATUM);
        strip.setTextColor(COL_FG);
        strip.drawString(nets[index].ssid, 20, SY(y + SCAN_ROW_H / 2));
        if (!nets[index].open)
        {
            strip.drawCircle(391, SY(y + 16), 5, COL_FG);
            strip.fillRect(384, SY(y + 17), 15, 11, COL_FG);
        }
        drawSignalBars(412, y + 30, rssiToBars(nets[index].rssi));
    }

    if (scan_page > 0)
        drawButton(BTN_PREV, "PREV", false);
    if (scan_page < scanPageCount() - 1)
        drawButton(BTN_NEXT, "NEXT", false);
    drawButton(BTN_RESCAN, "RESCAN", false);
    drawButton(BTN_OTHER, "OTHER", false);
}

void drawKeyboardField()
{
    if (!stripOverlaps(KB_FIELD_Y, KB_FIELD_H))
        return;
    const int x = 8;
    const int w = kb_secret ? 384 : 464;
    crtRect(x, KB_FIELD_Y, w, KB_FIELD_H, COL_BOX, COL_BOX_SCAN);
    strip.drawRect(x, SY(KB_FIELD_Y), w, KB_FIELD_H, COL_FG);

    // Show the end of the text when it doesn't all fit, followed by a cursor
    char shown[40];
    int len = strlen(kb_text);
    int visible = (w - 16) / 15 - 1;
    int n = 0;
    for (int i = max(0, len - visible); i < len; i++)
    {
        shown[n++] = kb_masked ? '*' : kb_text[i];
    }
    shown[n++] = '_';
    shown[n] = 0;

    strip.setFreeFont(&FreeMonoBold12pt7b);
    strip.setTextDatum(ML_DATUM);
    strip.setTextColor(COL_FG);
    strip.drawString(shown, x + 8, SY(KB_FIELD_Y + KB_FIELD_H / 2));
}

void drawKeyboardScreen()
{
    if (stripOverlaps(0, KB_FIELD_Y))
    {
        strip.setFreeFont(&FreeMonoBold9pt7b);
        strip.setTextDatum(ML_DATUM);
        strip.setTextColor(COL_FG);
        strip.drawString(kb_label, 8, SY(13));
    }

    drawKeyboardField();
    if (kb_secret)
        drawButton(BTN_SHOW, kb_masked ? "SHOW" : "HIDE", false);

    for (int row = 0; row < 4; row++)
    {
        const char *keys = KB_LAYERS[kb_layer][row];
        for (int col = 0; keys[col]; col++)
        {
            const char label[2] = {keys[col], 0};
            Button key = {KB_ROW_X[row] + col * KB_KEY_PITCH, KB_Y + row * KB_PITCH, KB_KEY_W, KB_KEY_H};
            drawButton(key, label, false);
        }
    }

    drawButton(BTN_DEL, "DEL", false);
    drawButton(BTN_SHIFT, "SHIFT", kb_layer == KB_UPPER);
    drawButton(BTN_SYM, "SYM", kb_layer == KB_SYMBOLS);
    drawButton(BTN_SPACE, "SPACE", false);
    drawButton(BTN_CANCEL, "CANCEL", false);
    drawButton(BTN_OK, "OK", false);
}

// Draw everything that overlaps the current strip
void renderScene()
{
    crtRect(0, 0, SCREEN_W, SCREEN_H, COL_BG, COL_BG_SCAN);

    if (setup_active)
    {
        if (setup_screen == SETUP_SCAN)
            drawScanScreen();
        else if (setup_screen == SETUP_KEYBOARD)
            drawKeyboardScreen();
        else
            drawMessageScreen();
        return;
    }

    if (stripOverlaps(0, HEADER_H))
        drawHeader();

    if (stripOverlaps(CLOCK_Y, GLYPH_H))
    {
        if (synced)
        {
            drawClockDigits();
        }
        else
        {
            strip.setFreeFont(&FreeMonoBold12pt7b);
            strip.setTextDatum(MC_DATUM);
            strip.setTextColor(COL_FG);
            strip.drawString(message, SCREEN_W / 2, SY(CLOCK_Y + GLYPH_H / 2));
        }
    }

    if (synced && stripOverlaps(STATUS_Y, STATUS_H))
        drawStatusBar();
}

// Brighten strip rows that fall inside the refresh band, strongest at its leading edge
void applyBand(int top, int bottom)
{
    uint16_t *buf = (uint16_t *)strip.getPointer();
    for (int row = max(top, band_y); row < min(bottom, band_y + BAND_H); row++)
    {
        int t = row - band_y;
        int add = t >= BAND_H - 2 ? 90 : 6 + 50 * t * t / (BAND_H * BAND_H);
        uint16_t *line = buf + SY(row) * SCREEN_W;
        for (int x = 0; x < SCREEN_W; x++)
        {
            uint16_t c = (line[x] >> 8) | (line[x] << 8); // Sprite stores pixels byte-swapped
            int r = min(255, ((c >> 8) & 0xF8) + add / 2);
            int g = min(255, ((c >> 3) & 0xFC) + add);
            int b = min(255, ((c << 3) & 0xF8) + add / 2);
            c = tft.color565(r, g, b);
            line[x] = (c >> 8) | (c << 8);
        }
    }
}

// Redraw a range of screen rows, including the band effect
void renderRows(int y, int h)
{
    int end = min(y + h, SCREEN_H);
    for (int top = max(y, 0); top < end; top += STRIP_H)
    {
        int rows = min(STRIP_H, end - top);
        strip_y = top;
        renderScene();
        applyBand(top, top + rows);
        strip.pushSprite(0, top, 0, 0, SCREEN_W, rows);
    }
}

void updateBand()
{
    static uint32_t last_frame = 0;
    uint32_t now = millis();
    bool offscreen = band_y >= SCREEN_H;

    if (now - last_frame < (offscreen ? BAND_PAUSE_MS : BAND_FRAME_MS))
        return;
    last_frame = now;

    if (offscreen)
    {
        band_y = -BAND_H;
        return;
    }

    // Redraw the rows the band just left together with its new position
    band_y += BAND_STEP;
    renderRows(band_y - BAND_STEP, BAND_H + BAND_STEP);
}

void updateClock()
{
    static int last_sec = -1;
    struct tm now;

    if (!getLocalTime(&now, 0))
    {
        const char *msg = WiFi.isConnected() ? "SYNCING TIME..." : "CONNECTING TO WI-FI...";
        if (msg != message)
        {
            message = msg;
            renderRows(CLOCK_Y, GLYPH_H);
        }
        return;
    }
    if (now.tm_sec == last_sec)
        return;
    last_sec = now.tm_sec;

    int new_digits[4] = {now.tm_hour / 10, now.tm_hour % 10, now.tm_min / 10, now.tm_min % 10};
    bool changed = !synced || memcmp(new_digits, digits, sizeof(digits)) != 0;
    memcpy(digits, new_digits, sizeof(digits));
    colon_on = now.tm_sec % 2 == 0;

    if (changed || now.tm_sec % 10 == 0)
    {
        strftime(status_date, sizeof(status_date), "%Y-%m-%d %H:%M", &now);
        wifi_bars = 0;
        if (WiFi.isConnected())
            wifi_bars = rssiToBars(WiFi.RSSI());
        renderRows(STATUS_Y, STATUS_H);
    }

    if (changed)
    {
        synced = true;
        renderRows(CLOCK_Y, GLYPH_H);
    }
    else
    {
        renderRows(CLOCK_Y + 2 * CELL, 3 * CELL); // Colon only
    }
}

// Reports one tap per press, in screen coordinates
bool readTap(int &x, int &y)
{
    static bool was_down = false;
    static uint32_t last_down = 0;

    bool down = touch.touched();
    uint32_t now = millis();
    bool pressed = down && !was_down && now - last_down > 80; // Ignore bounce on release
    was_down = down;
    if (down)
        last_down = now;
    if (!pressed)
        return false;

    // Let the pressure settle; the first samples of a press are unreliable
    delay(15);
    TS_Point p = touch.getPoint();
    x = constrain(map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, SCREEN_W - 1), 0, SCREEN_W - 1);
    y = constrain(map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_H - 1), 0, SCREEN_H - 1);
    if (TOUCH_FLIP_X)
        x = SCREEN_W - 1 - x;
    if (TOUCH_FLIP_Y)
        y = SCREEN_H - 1 - y;
#if TOUCH_DEBUG
    Serial.printf("Touch raw x=%d y=%d z=%d -> screen x=%d y=%d\n", p.x, p.y, p.z, x, y);
#endif
    return true;
}

void waitForTap(int &x, int &y)
{
    while (!readTap(x, y))
    {
        delay(10);
    }
}

bool hit(const Button &b, int x, int y)
{
    return x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h;
}

void showSetupScreen(SetupScreen screen)
{
    setup_screen = screen;
    renderRows(0, SCREEN_H);
}

void showMessage(const char *title, const char *line1, const char *line2,
                 const char *left, const char *right)
{
    msg_title = title;
    msg_line1 = line1;
    msg_line2 = line2;
    msg_left = left;
    msg_right = right;
    showSetupScreen(SETUP_MESSAGE);
}

// Wait for one of the message screen's two buttons; true means the left one
bool waitForChoice()
{
    while (true)
    {
        int x, y;
        waitForTap(x, y);
        if (hit(BTN_LEFT, x, y))
            return true;
        if (hit(BTN_RIGHT, x, y))
            return false;
    }
}

char keyboardKeyAt(int x, int y)
{
    if (y < KB_Y)
        return 0;
    int row = (y - KB_Y) / KB_PITCH;
    if (row > 3 || x < KB_ROW_X[row])
        return 0;
    const char *keys = KB_LAYERS[kb_layer][row];
    int col = (x - KB_ROW_X[row]) / KB_KEY_PITCH;
    return col < (int)strlen(keys) ? keys[col] : 0;
}

// Let the user type a value. Returns false if cancelled, leaving out unchanged.
bool editText(const char *label, char *out, int max_len, bool secret)
{
    strlcpy(kb_label, label, sizeof(kb_label));
    kb_text[0] = 0;
    kb_max = max_len;
    kb_layer = KB_LOWER;
    kb_secret = secret;
    kb_masked = secret;
    showSetupScreen(SETUP_KEYBOARD);

    while (true)
    {
        int x, y;
        waitForTap(x, y);
        int len = strlen(kb_text);
        char typed = 0;

        if (hit(BTN_OK, x, y))
        {
            if (!secret && len == 0)
                continue; // A network name can't be empty
            strlcpy(out, kb_text, max_len + 1);
            return true;
        }
        if (hit(BTN_CANCEL, x, y))
            return false;

        if (hit(BTN_SHIFT, x, y) || hit(BTN_SYM, x, y))
        {
            int layer = hit(BTN_SHIFT, x, y) ? KB_UPPER : KB_SYMBOLS;
            kb_layer = kb_layer == layer ? KB_LOWER : layer;
            renderRows(KB_Y, SCREEN_H - KB_Y);
        }
        else if (secret && hit(BTN_SHOW, x, y))
        {
            kb_masked = !kb_masked;
            renderRows(KB_FIELD_Y, KB_FIELD_H);
        }
        else if (hit(BTN_DEL, x, y))
        {
            if (len > 0)
            {
                kb_text[len - 1] = 0;
                renderRows(KB_FIELD_Y, KB_FIELD_H);
            }
        }
        else if (hit(BTN_SPACE, x, y))
        {
            typed = ' ';
        }
        else
        {
            typed = keyboardKeyAt(x, y);
        }

        if (typed && len < kb_max)
        {
            kb_text[len] = typed;
            kb_text[len + 1] = 0;
            renderRows(KB_FIELD_Y, KB_FIELD_H);
        }
    }
}

// Fill nets with visible networks, strongest first, one entry per SSID
void scanForNetworks()
{
    showMessage("SCANNING...", "", "", nullptr, nullptr);

    int found = WiFi.scanNetworks();
    net_count = 0;
    scan_page = 0;
    for (int i = 0; i < found; i++)
    {
        String ssid = WiFi.SSID(i);
        int rssi = WiFi.RSSI(i);
        if (ssid.length() == 0)
            continue; // Hidden network, reachable through OTHER

        int slot = -1;
        for (int j = 0; j < net_count; j++)
        {
            if (ssid == nets[j].ssid)
                slot = j;
        }
        if (slot >= 0 && rssi <= nets[slot].rssi)
            continue;
        if (slot < 0)
        {
            if (net_count == SCAN_MAX && rssi <= nets[SCAN_MAX - 1].rssi)
                continue;
            slot = net_count < SCAN_MAX ? net_count++ : SCAN_MAX - 1;
        }

        strlcpy(nets[slot].ssid, ssid.c_str(), sizeof(nets[slot].ssid));
        nets[slot].rssi = rssi;
        nets[slot].open = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;

        for (; slot > 0 && nets[slot - 1].rssi < nets[slot].rssi; slot--)
        {
            ScanEntry swapped = nets[slot - 1];
            nets[slot - 1] = nets[slot];
            nets[slot] = swapped;
        }
    }
    WiFi.scanDelete();
}

// Let the user pick a network and enter its password into cand_ssid and cand_pass
void chooseNetwork()
{
    char label[sizeof(kb_label)];

    scanForNetworks();
    while (true)
    {
        showSetupScreen(SETUP_SCAN);
        int x, y;
        waitForTap(x, y);

        if (hit(BTN_RESCAN, x, y))
        {
            scanForNetworks();
        }
        else if (hit(BTN_OTHER, x, y))
        {
            if (!editText("NETWORK NAME", cand_ssid, SSID_MAX, false))
                continue;
            snprintf(label, sizeof(label), "PASSWORD FOR %s (EMPTY IF OPEN)", cand_ssid);
            if (editText(label, cand_pass, PASS_MAX, true))
                return;
        }
        else if (hit(BTN_PREV, x, y))
        {
            if (scan_page > 0)
                scan_page--;
        }
        else if (hit(BTN_NEXT, x, y))
        {
            if (scan_page < scanPageCount() - 1)
                scan_page++;
        }
        else if (y >= SCAN_Y && y < SCAN_Y + SCAN_ROWS * SCAN_PITCH)
        {
            int index = scan_page * SCAN_ROWS + (y - SCAN_Y) / SCAN_PITCH;
            if (index >= net_count)
                continue;

            strlcpy(cand_ssid, nets[index].ssid, sizeof(cand_ssid));
            if (nets[index].open)
            {
                cand_pass[0] = 0;
                return;
            }
            snprintf(label, sizeof(label), "PASSWORD FOR %s", cand_ssid);
            if (editText(label, cand_pass, PASS_MAX, true))
                return;
        }
    }
}

bool connectToCandidate()
{
    showMessage("CONNECTING...", cand_ssid, "", nullptr, nullptr);

    WiFi.begin(cand_ssid, cand_pass[0] ? cand_pass : nullptr);
    uint32_t start = millis();
    while (millis() - start < WIFI_CONNECT_TIMEOUT_MS)
    {
        if (WiFi.status() == WL_CONNECTED)
            return true;
        delay(100);
    }
    WiFi.disconnect();
    return false;
}

// Blocks until connected. Credentials are only saved once they have worked.
void runWifiSetup()
{
    WiFi.persistent(false); // Credentials live in our own namespace, not the Wi-Fi driver's
    WiFi.mode(WIFI_STA);

    Preferences prefs;
    prefs.begin("wifi", false);
    prefs.getString("ssid", "").toCharArray(cand_ssid, sizeof(cand_ssid));
    prefs.getString("pass", "").toCharArray(cand_pass, sizeof(cand_pass));
    String saved_ssid = cand_ssid;
    String saved_pass = cand_pass;

    setup_active = true;

    bool ready = false;
    if (cand_ssid[0])
    {
        char mask[21];
        int stars = min((int)strlen(cand_pass), (int)sizeof(mask) - 1);
        memset(mask, '*', stars);
        mask[stars] = 0;
        showMessage("CONNECT TO SAVED WI-FI?", cand_ssid, stars ? mask : "NO PASSWORD", "CONNECT", "CHANGE");
        ready = waitForChoice();
    }

    while (true)
    {
        if (!ready)
            chooseNetwork();
        if (connectToCandidate())
            break;
        showMessage("CONNECTION FAILED", cand_ssid, "", "RETRY", "CHANGE");
        ready = waitForChoice();
    }

    // Avoid wearing the flash, since this runs on every boot
    if (saved_ssid != cand_ssid)
        prefs.putString("ssid", cand_ssid);
    if (saved_pass != cand_pass)
        prefs.putString("pass", cand_pass);
    prefs.end();

    setup_active = false;
    renderRows(0, SCREEN_H);
}

void setup()
{
    Serial.begin(115200);

    tft.init();
    tft.setRotation(1); // Landscape, 480x320

    // Touch shares the display's SPI bus (SCK 12, MOSI 13, MISO 4)
    touch.begin(tft.getSPIinstance());
    touch.setRotation(1);

    COL_BG = tft.color565(6, 26, 12);
    COL_BG_SCAN = tft.color565(2, 14, 6);
    COL_FG = tft.color565(70, 255, 120);
    COL_FG_SCAN = tft.color565(40, 175, 80);
    COL_DIM = tft.color565(30, 110, 55);
    COL_BOX = tft.color565(20, 80, 40);
    COL_BOX_SCAN = tft.color565(14, 58, 28);

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
