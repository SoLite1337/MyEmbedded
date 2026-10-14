#include "Led.h"
#include "config.h"
#include <Arduino.h>

void Led::init(u8_t p)
{
    pin = p;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void Led::set(bool on)
{
    blinkMode = false;
    state     = on;
    digitalWrite(pin, on ? HIGH : LOW);
}

void Led::off()
{
    set(false);
}

void Led::blink(u32_t interval_ms)
{
    blinkMode     = true;
    blinkInterval = interval_ms;
    lastToggle    = millis();
    state         = true;
    digitalWrite(pin, HIGH);
}

void Led::poll()
{
    if (!blinkMode) return;

    if (millis() - lastToggle >= blinkInterval)
    {
        lastToggle = millis();
        state = !state;
        digitalWrite(pin, state ? HIGH : LOW);
    }
}