// Strip renderer and CRT effects (scanlines, rolling refresh band)

void initPalette()
{
    COL_BG = tft.color565(6, 26, 12);
    COL_BG_SCAN = tft.color565(2, 14, 6);
    COL_FG = tft.color565(70, 255, 120);
    COL_FG_SCAN = tft.color565(40, 175, 80);
    COL_DIM = tft.color565(30, 110, 55);
    COL_BOX = tft.color565(20, 80, 40);
    COL_BOX_SCAN = tft.color565(14, 58, 28);
}

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
