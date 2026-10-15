#pragma once
#include <Arduino.h>
#include "config.h"

class Ldr
{
private:
    u16_t readings[LDR_FILTER_N];
    u8_t  idx          = 0;
    u32_t sum          = 0;
    u16_t lastValue    = 0;
    u32_t lastPollTime = 0;
    u32_t lastLogTime  = 0;

    u16_t movingAvg(u16_t raw);
    void  filterInit(u16_t initData);

public:
    void  init();
    void  poll();
    u16_t getValue();
};