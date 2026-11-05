// Clock state updates from NTP-synced local time

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
