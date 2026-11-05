// Drawing of the clock and Wi-Fi setup screens into the render strip

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
