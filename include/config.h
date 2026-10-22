#pragma once
#include <Arduino.h>

struct Config {
    //LED
    static constexpr uint8_t  LED_PIN        = 16;
    static constexpr uint32_t BLINK_MS       = 500;

    //BUTTON
    static constexpr uint8_t  BTN_PIN        = 17;
    static constexpr uint32_t DEBOUNCE_MS    = 20;

    //SERIAL
    static constexpr uint32_t SERIAL_BAUD    = 115200;
    static constexpr uint32_t PRINT_EVERY    = 1000;
};