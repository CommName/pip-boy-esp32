// Interactive Wi-Fi setup: saved network prompt, network scan, on-screen keyboard

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
