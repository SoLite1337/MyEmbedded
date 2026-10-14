#include <Arduino.h>
#include "config.h"
#include "Button.h"
#include "Ldr.h"
#include "Led.h"

//Объекты
Button btnArm;      // кнопка Arm
Button btnReset;    // кнопка Ack
Ldr    ldr;
Led    ledGreen;
Led    ledRed;

enum SystemState { MASKED, ARMED, ALARM };
SystemState currentState = MASKED;
SystemState lastState    = (SystemState)-1;  //исп. знач. -1 = ка "не задано", чтобы применить выходы на старте

u32_t alarmStartTime = 0;

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
    Serial.println("Initial state: MASKED");
}

void loop()
{
    //Опрос входов
    ldr.poll();
    btnArm.poll();
    btnReset.poll();
    ledGreen.poll();
    ledRed.poll();

    //Логика переходов
    switch (currentState)
    {
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
        //Тревога. Выход только по ACK.
        //Ни btnArm, ни LDR на состояние не влияют.
        if (btnReset.wasPressed())
        {
            u32_t latency = millis() - btnReset.getPressTime();
            Serial.printf("Reset latency: %lu ms\n", latency);
            currentState = ARMED;
        }
        break;
    }

    //Отработка выходов при смене состояния
    if (currentState != lastState)
    {
        switch (currentState)
        {
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