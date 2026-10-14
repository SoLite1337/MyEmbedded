#include "Ldr.h"
#include "config.h"
#include <Arduino.h>

void Ldr::init()
{
    pinMode(LDR, INPUT);
    filterInit(analogRead(LDR));
}

void Ldr::poll()
{
    if (millis() - lastPollTime < LDR_PERIOD_MS) return;
    lastPollTime = millis();

    u16_t adc_value          = analogRead(LDR);
    u16_t filtered_adc_value = movingAvg(adc_value);

    //яркий свет = тревога
    ldrState = (filtered_adc_value > LDR_THRESHOLD);

    Serial.printf("LDR raw=%d filt=%d state=%d\n",
                  adc_value, filtered_adc_value, ldrState);
                  
}

bool Ldr::get_state()
{
    return ldrState;
}

void Ldr::filterInit(u16_t initData)
{
    sum = 0;
    for (u8_t i = 0; i < LDR_FILTER_N; i++)
    {
        readings[i] = initData;
        sum        += initData;
    }
}

u16_t Ldr::movingAvg(u16_t rawData)
{
    sum -= readings[idx];
    sum += rawData;
    readings[idx] = rawData;

    idx++;
    if (idx >= LDR_FILTER_N) idx = 0;

    return sum / LDR_FILTER_N;
}