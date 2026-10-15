#include "Ldr.h"
#include <Arduino.h>

void Ldr::init()
{
    pinMode(LDR_PIN, INPUT);
    filterInit(analogRead(LDR_PIN));
}

void Ldr::poll()
{
    if (millis() - lastPollTime < LDR_PERIOD_MS) return;
    lastPollTime = millis();

    u16_t raw = analogRead(LDR_PIN);
    lastValue = movingAvg(raw);

    if (millis() - lastLogTime >= LDR_LOG_PERIOD_MS)
    {
        lastLogTime = millis();
        Serial.printf("LDR raw=%u filt=%u\n", raw, lastValue);
    }
}

u16_t Ldr::getValue()
{
    return lastValue;
}

void Ldr::filterInit(u16_t initData)
{
    sum = 0;
    for (u8_t i = 0; i < LDR_FILTER_N; i++)
    {
        readings[i] = initData;
        sum        += initData;
    }
    lastValue = initData;
}

u16_t Ldr::movingAvg(u16_t raw)
{
    sum -= readings[idx];
    sum += raw;
    readings[idx] = raw;

    idx++;
    if (idx >= LDR_FILTER_N) idx = 0;

    return sum / LDR_FILTER_N;
}