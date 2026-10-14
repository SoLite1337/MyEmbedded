#include "Button.h"
#include "config.h"
#include <Arduino.h>

void Button::init(u8_t p)
{
    pin = p;
    pinMode(pin, INPUT_PULLDOWN);
}

void Button::poll()
{
    if (millis() - lastPollTime < BTN_PERIOD_MS) return;
    lastPollTime = millis();

    bool raw = digitalRead(pin);

    // Дебаунс через периодический опрос
    if (raw != state)
    {
        state = raw;
        if (state)
        {
            pressedFlag = true;
            pressTime   = millis();
        }
    }
}

bool Button::get_state()
{
    return state;
}

bool Button::wasPressed()
{
    if (pressedFlag)
    {
        pressedFlag = false;
        return true;
    }
    return false;
}

u32_t Button::getPressTime()
{
    return pressTime;
}