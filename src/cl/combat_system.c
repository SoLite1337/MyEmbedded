#include "cl/combat_system.h"
#include "dl/led.h"
#include "core/event_bus.h"

extern led_t g_led_green;
static cs_state_t s_state;

void combat_system_init(void) 
{ 
    s_state = CS_ST_IDLE; 
}

void combat_system_tick(void) 
{
    app_event_t evt;
    while (event_bus_pop(&evt)) 
    {
        if (evt == EVT_BTN_COMBAT_LONG && s_state == CS_ST_IDLE) 
        {
            led_on(&g_led_green);
            s_state = CS_ST_LOCKED;
        } 
        else if (evt == EVT_BTN_COMBAT_SHORT && s_state == CS_ST_LOCKED) 
        {
            led_blink(&g_led_green, CS_BLINK_COUNT, CS_BLINK_MS, CS_BLINK_MS);
            s_state = CS_ST_LAUNCHING;
        }
    }

    if (s_state == CS_ST_LAUNCHING) 
    {
        if (!led_is_blinking(&g_led_green)) 
        {
            led_off(&g_led_green);
            s_state = CS_ST_IDLE;
        }
    }
}

// Функции для проверки состояния боевой системы
uint8_t combat_system_is_active(void) { return (s_state != CS_ST_IDLE) ? 1u : 0u; }
cs_state_t combat_system_get_state(void) { return s_state; }