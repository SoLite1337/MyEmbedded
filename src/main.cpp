#include <Arduino.h>
#include "config.h"
#include "Ldr.h"
#include "Relay.h"

Ldr   ldr;
Relay relay;

void setup()
{
    Serial.begin(115200);
    delay(2000);

    ldr.init();
    relay.init();
    Serial.printf("RELAY_PIN = %d\n", RELAY_PIN);
    Serial.printf("LDR_PIN   = %d\n", LDR_PIN);   
    Serial.println("=== Twilight Switch (Dusk/Dawn) ===");
    Serial.printf("THRESHOLD_DARK=%d  THRESHOLD_LIGHT=%d\n",
                  THRESHOLD_DARK, THRESHOLD_LIGHT);
}

void loop()
{
    ldr.poll();
    relay.update(ldr.getValue());
}