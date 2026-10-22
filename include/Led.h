#pragma once
#include <Arduino.h>

enum class LedState {
    Off,
    On,
    Blinking
};

class Led {
public:
    void init(uint8_t p, uint32_t interval);
    void set(LedState s);
    void update();              // вызвать в loop()
    LedState nextMode();        // Blinking -> On -> Off -> Blinking
    LedState getState();

private:
    uint8_t  pin           = 0;
    uint32_t blinkInterval = 500;
    uint32_t lastToggle    = 0;
    bool     blinkOn       = false;
    LedState state         = LedState::Off;

    void writeOutput();
};