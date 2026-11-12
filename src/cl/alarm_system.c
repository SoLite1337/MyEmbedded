#include "cl/alarm_system.h"
#include "dl/led.h"
#include "core/event_bus.h"

extern led_t g_led_yellow;
static as_state_t s_state;

void alarm_system_init(void) 
{ 
    s_state = AS_ST_IDLE; 
}


// Основной цикл системы сигнализации
void alarm_system_tick(void) 
{
    app_event_t evt;
    
    while (event_bus_pop(&evt)) 
    {
        if (evt == EVT_BTN_SENSOR_SHORT && s_state == AS_ST_IDLE) 
        {
            led_on(&g_led_yellow);
            s_state = AS_ST_WARNING;
        } 
        else if (evt == EVT_BTN_SENSOR_LONG && (s_state == AS_ST_IDLE || s_state == AS_ST_WARNING)) 
        {
            led_blink(&g_led_yellow, LED_BLINK_FOREVER, AS_BLINK_MS, AS_BLINK_MS);
            s_state = AS_ST_ALARM;
        }
        else if (evt == EVT_BTN_SENSOR_SHORT && s_state == AS_ST_WARNING) 
        {
            led_off(&g_led_yellow);
            s_state = AS_ST_IDLE;
        }
        else if (evt == EVT_THERMAL_BURST_DONE) 
        { // Слухаємо подію від теплових пасток!
            if (s_state == AS_ST_WARNING || s_state == AS_ST_ALARM) 
            {
                led_off(&g_led_yellow);
                s_state = AS_ST_IDLE;
            }
        }
    }
}

// Функції для перевірки стану системи сигналізації
uint8_t alarm_system_is_active(void) 
{ 
    return (s_state != AS_ST_IDLE) ? 1u : 0u; 
}
as_state_t alarm_system_get_state(void) 
{ 
    return s_state; 
}