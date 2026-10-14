#pragma once
#include <Arduino.h>

class Button
{
private:
    u8_t  pin = 0;
    u32_t lastPollTime = 0;
    bool  state        = false;   // текущее состояние (после дебаунса)
    bool  pressedFlag  = false;   // одноразовый флаг нажатия (фронт)
    u32_t pressTime    = 0;       // момент нажатия (для latency)

public:
    void  init(u8_t p);
    void  poll();
    bool  get_state();
    bool  wasPressed();           // true при фронте нажатия
    u32_t getPressTime();
};