#include <Arduino.h>
#include "Config.h"
#include "Led.h"
#include "Button.h"

Led    led;
Button button;

// счетчик для измерения времени в loop()
uint32_t loops  = 0;
uint32_t sumUs  = 0;

void setup() {
    Serial.begin(Config::SERIAL_BAUD);
    delay(2000);

    led.init(Config::LED_PIN, Config::BLINK_MS);
    button.init(Config::BTN_PIN, Config::DEBOUNCE_MS);
    led.set(LedState::Blinking);
    Serial.printf("LED=%d  BTN=%d\n", Config::LED_PIN, Config::BTN_PIN);
}

void loop() {
    uint32_t t0 = micros();

    button.update();
    led.update();

    if (button.wasPressed()) 
    {
        LedState m = led.nextMode();
        Serial.print("Mode: ");
        if (m == LedState::Off)
        {
          Serial.println("OFF");
        }
        else if (m == LedState::On)       
        {
          Serial.println("ON");
        }
        else
        {
          Serial.println("BLINK");
        }
    }

    sumUs += micros() - t0;
    loops++;

    if (loops >= Config::PRINT_EVERY) 
    {
        uint32_t avg = sumUs / loops;
        Serial.printf("loops=%d  avg=%d us\n", loops, avg);
        loops = 0;
        sumUs = 0;
    }
}