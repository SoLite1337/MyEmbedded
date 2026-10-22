#pragma once
#include <Arduino.h>

class Button {
public:
    void init(uint8_t p, uint32_t debounceTime);
    void update();
    bool wasPressed();

    static void IRAM_ATTR isr();

private:
    static volatile bool irqFlag; // меняется в ISR

    uint8_t  pin           = 0;
    uint32_t debounce      = 20;
    uint32_t lastChange    = 0;
    bool     lastReading   = true;   // pull-up
    bool     pressEvent    = false;
};