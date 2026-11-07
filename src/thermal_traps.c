#include "thermal_traps.h"
#include "led.h"
#include "button.h"

// НАЗНАЧЕНИЕ:
// -  Подсистема «тепловых ловушек» (красный LED). Запускается по LONG-событию кнопки BTN_THERMAL.
//   Выполняет серию из 4 вспышек: 100 мс ON / 250 мс OFF.
// -  По завершении поднимает флаг burst_done, который читают другие модули
// -  Реализовано как FSM (tt_state_t).


extern button_t g_btn_thermal;
extern led_t    g_led_red;

static tt_state_t       s_state;
static uint32_t         s_timer;
static uint8_t          s_count;
static volatile uint8_t s_burst_done;

void thermal_traps_init(void) // инициализация (LED OFF, сброс флагов)
{
    s_state      = TT_ST_IDLE;
    s_timer      = 0;
    s_count      = 0;
    s_burst_done = 0;
}

void thermal_traps_tick(void) // FSM тепловых ловушек, вызывается из system_tick()
{
    btn_event_t evt = button_get_event(&g_btn_thermal);

    // Запуск возможен только из IDLE и только LONG-событием.
    if (evt == BTN_EVT_LONG_PRESS && s_state == TT_ST_IDLE) 
    {
        s_state = TT_ST_BURST_ON;
        s_timer = TT_ON_MS;
        s_count = 0;
        led_on(&g_led_red);
        return;
    }

    switch (s_state) // FSM тепловых ловушек
    {
        case TT_ST_IDLE:
            break;

        case TT_ST_BURST_ON: // текущая вспышка светит
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
                    // Вся серия завершена, сигнал наружу.
                    s_state      = TT_ST_IDLE;
                    s_burst_done = 1;
                } 
                else 
                {
                    s_state = TT_ST_BURST_OFF;
                    s_timer = TT_OFF_MS;
                }
            }
            break;

        case TT_ST_BURST_OFF: // пауза между вспышками
            if (s_timer > 0) 
            {
                s_timer--;
            }
            if (s_timer == 0) 
            {
                led_on(&g_led_red);
                s_state = TT_ST_BURST_ON;
                s_timer = TT_ON_MS;
            }
            break;
    }
}

uint8_t thermal_traps_consume_burst_done(void)    // есть ли флаг burst_done? Если есть, то сбрасываем его и ret 1, иначе ret 0.
{
    if (s_burst_done) 
    { 
        s_burst_done = 0; return 1; 
    }
    return 0;
}

uint8_t thermal_traps_is_active(void)               // не IDLE?
{
    return (s_state != TT_ST_IDLE) ? 1u : 0u;
}

tt_state_t thermal_traps_get_state(void)            // получить текущее состояние FSM тепловых ловушек
{
    return s_state;
}