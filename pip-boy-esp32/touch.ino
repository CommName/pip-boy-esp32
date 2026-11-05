// Touch input

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
