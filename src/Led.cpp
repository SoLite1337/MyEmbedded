#include "Led.h"

void Led::init(uint8_t p, uint32_t interval) {
    pin = p;
    blinkInterval = interval;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    state = LedState::Off;
}

void Led::set(LedState s) {
    state = s;
    blinkOn = false;
    lastToggle = millis();
    writeOutput();
}

void Led::update() {
    if (state != LedState::Blinking) 
    {
    return;
    }
    if (millis() - lastToggle >= blinkInterval) {
        lastToggle = millis();
        blinkOn = !blinkOn;
        writeOutput();
    }
}

LedState Led::nextMode() {
    if (state == LedState::Blinking)     
    {
        set(LedState::On);
    }
    else if (state == LedState::On)      
    {
        set(LedState::Off);
    }
    else
    {
        set(LedState::Blinking);
    }
    return state;
}

LedState Led::getState() {
    return state;
}

void Led::writeOutput() {
    bool on;
    if (state == LedState::On)       
    {
        on = true;
    }
    else if (state == LedState::Off) 
    {
        on = false;
    }
    else
    {
        on = blinkOn;
    }

    digitalWrite(pin, on ? HIGH : LOW);
}