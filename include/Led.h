#pragma once
#include <Arduino.h>

class Led
{
private:
    u8_t  pin          = 0;
    bool  state        = false;
    bool  blinkMode    = false;
    u32_t blinkInterval = 100;
    u32_t lastToggle   = 0;

public:
    void init(u8_t p);
    void set(bool on);       // постоянное вкл/выкл (останавливает blink)
    void off();
    void blink(u32_t interval_ms);
    void poll();             // вызывать в loop() для неблокирующего мигания
};