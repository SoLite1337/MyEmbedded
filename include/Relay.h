#pragma once
#include <Arduino.h>
#include "config.h"

class Relay
{
private:
    bool  state        = false;   // логическое состояние (вкл/выкл нагрузки)
    u32_t lastUpdateMs = 0;

    void write(bool on);

public:
    void init();
    void update(u16_t adcValue);  // применяет гистерезис
    bool isOn();
};