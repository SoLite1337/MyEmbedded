#include <Arduino.h>
#include "config.h"
#include "Button.h"
#include "Ldr.h"
#include "Led.h"

Button btnArm;
Button btnReset;
Ldr    ldr;
Led    ledGreen;
Led    ledRed;

enum SystemState {
    MASKED = 0,
    INIT   = 1,
    ARMED  = 2,
    ALARM  = 3
};

SystemState currentState = INIT;
SystemState lastState    = (SystemState)-1;
u32_t alarmStartTime = 0;
u32_t initStartTime  = 0;

void setup()
{
    Serial.begin(115200);
    delay(1000);
    btnArm.init(BTN_ARM);
    btnReset.init(BTN_RESET);
    ldr.init();
    ledGreen.init(LED_GREEN);
    ledRed.init(LED_RED);
    Serial.println("=== Laser Detection System ===");
    Serial.println("Initial state: INIT");
    initStartTime = millis();
}

void loop()
{
    // Опрос входов
    ldr.poll();
    btnArm.poll();
    btnReset.poll();
    ledGreen.poll();
    ledRed.poll();

    switch (currentState)
    {
      case INIT:
        btnArm.wasPressed();
        btnReset.wasPressed();
        if (millis() - initStartTime >= INIT_PERIOD_MS)
        {
            currentState = MASKED;
        }
        break;

      case MASKED:
        if (btnArm.wasPressed())
        {
            currentState = ARMED;
        }
        break;

      case ARMED:
        if (btnArm.wasPressed())
        {
            currentState = MASKED;
        }
        else if (ldr.get_state())
        {
            currentState   = ALARM;
            alarmStartTime = millis();
        }
        break;

      case ALARM:
        btnArm.wasPressed();
        if (btnReset.wasPressed())
        {
            u32_t latency = millis() - btnReset.getPressTime();
            Serial.printf("Reset latency: %lu ms\n", latency);
            currentState = ARMED;
        }
        break;
    }

    // Отработка выходов при смене состояния
    if (currentState != lastState)
    {
        switch (currentState)
        {
        case INIT:
            ledGreen.blink(500);   // не быстрое мигання = инициализация
            ledRed.off();
            Serial.println("State: INIT");
            break;

        case MASKED:
            ledGreen.set(false);
            ledRed.set(false);
            Serial.println("State: MASKED");
            break;

        case ARMED:
            ledGreen.set(true);
            ledRed.set(false);
            Serial.println("State: ARMED");
            break;

        case ALARM:
            ledGreen.set(false);
            ledRed.blink(BLINK_PERIOD_MS);
            Serial.println("State: ALARM (until ACK)");
            break;
        }
        lastState = currentState;
    }
}