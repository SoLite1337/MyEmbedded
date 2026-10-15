#include "Relay.h"
#include <Arduino.h>

void Relay::init()
{
    pinMode(RELAY_PIN, OUTPUT);
    state = false;
    write(false);
    Serial.println("Relay init: OFF");
}

void Relay::write(bool on)
{
#if RELAY_ACTIVE_HIGH
    digitalWrite(RELAY_PIN, on ? HIGH : LOW);
#else
    digitalWrite(RELAY_PIN, on ? LOW : HIGH);
#endif
}

void Relay::update(u16_t adcValue)
{
    // Тёмно: включаем реле, если ещё не включено
    if (adcValue < THRESHOLD_DARK && !state)
    {
        state = true;
        write(true);
        Serial.printf(">> DARK  (adc=%u) -> RELAY ON\n", adcValue);
    }
    // Светло: выключаем реле, если ещё включено
    else if (adcValue > THRESHOLD_LIGHT && state)
    {
        state = false;
        write(false);
        Serial.printf(">> LIGHT (adc=%u) -> RELAY OFF\n", adcValue);
    }
    // Между порогами — ничего не делаем (гистерезис)
}

bool Relay::isOn()
{
    return state;
}