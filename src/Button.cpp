#include "Button.h"

volatile bool Button::irqFlag = false;

void IRAM_ATTR Button::isr() {
    irqFlag = true;
}

void Button::init(uint8_t p, uint32_t debounceTime) {
    pin = p;
    debounce = debounceTime;
    pinMode(pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(pin), &Button::isr, CHANGE);
}

void Button::update() {
    if (!irqFlag) 
    {
        return;
    }
    irqFlag = false;

    uint32_t now = millis();
    if (now - lastChange < debounce) 
    {
        return;
    }
    lastChange = now;

    bool reading = digitalRead(pin);
    if (reading == LOW && lastReading == HIGH) 
    {
        pressEvent = true;
    }
    lastReading = reading;
}

bool Button::wasPressed() {
    if (pressEvent) 
    {
        pressEvent = false;
        return true;
    }
    return false;
}