#include "cl/thermal_traps.h"
#include "dl/led.h"
#include "core/event_bus.h"

extern led_t g_led_red; // Беремо доступ до LED (це допустимо, це виконавчий механізм)
static tt_state_t s_state;
static uint32_t   s_timer;
static uint8_t    s_count;

void thermal_traps_init(void) 
{ 
    s_state = TT_ST_IDLE; 
    s_timer = 0; 
    s_count = 0; 
}

void thermal_traps_tick(void) 
{
    app_event_t evt;
    

    //Обработка всех событий из шины событий
    while (event_bus_pop(&evt)) 
    {
        if (evt == EVT_BTN_THERMAL_LONG && s_state == TT_ST_IDLE) 
        {
            s_state = TT_ST_BURST_ON;
            s_timer = TT_ON_MS;
            s_count = 0;
            led_on(&g_led_red);
        }
        // Якщо подія не наша — ігноруємо (в майбутньому тут можна робити чергу підписок)
    }

    switch (s_state) 
    {
        case TT_ST_IDLE: 
        break;

        case TT_ST_BURST_ON:
            if (s_timer > 0) 
            {
                s_timer--;
            }
            if (s_timer == 0) 
            {
                led_off(&g_led_red);
                s_count++;
                if (s_count >= TT_COUNT) 
                {
                    s_state = TT_ST_IDLE;
                    event_bus_publish(EVT_THERMAL_BURST_DONE); // ПУБЛІКУЄМО СИСТЕМНУ ПОДІЮ
                } else {
                    s_state = TT_ST_BURST_OFF; s_timer = TT_OFF_MS;
                }
            }
            break;

        case TT_ST_BURST_OFF:
            if (s_timer > 0) 
            {
                s_timer--;
            }
            if (s_timer == 0) 
            {
                led_on(&g_led_red); s_state = TT_ST_BURST_ON; s_timer = TT_ON_MS;
            }
            break;
    }
}

// Проверка активности тепловых ловушек: 1 - активны, 0 - неактивны
uint8_t thermal_traps_is_active(void) 
{ 
    return (s_state != TT_ST_IDLE) ? 1u : 0u; 
}

tt_state_t thermal_traps_get_state(void) 
{ 
    return s_state; 
}