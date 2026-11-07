#include "combat_system.h"
#include "led.h"
#include "button.h"

//  НАЗНАЧЕНИЕ:
//   - одно событие = один переход
//   - вся работа с LED идёт через объект g_led_green, т.е. доступ только из драйвера
//   - завершение серии LAUNCHING по состоянию LED-драйвера (led_is_blinking == 0).


extern button_t g_btn_combat;  // кнопка боевого режима
extern led_t    g_led_green;   // зеленый LED

static cs_state_t s_state;     // текущее состояние FSM боевого режима


void combat_system_init(void)  // инициализация, перевод в IDLE 
{ 
    s_state = CS_ST_IDLE; 
}

void combat_system_tick(void)  // Один шаг FSM. Вызывать раз в 1 мс из app_tick() (TIM3 ISR).
{
    btn_event_t evt = button_get_event(&g_btn_combat);

    switch (s_state) 
    {
        case CS_ST_IDLE: // ждём LONG
            if (evt == BTN_EVT_LONG_PRESS) 
            {
                led_on(&g_led_green);       // цель захвачена
                s_state = CS_ST_LOCKED;
            }
            break;

        case CS_ST_LOCKED: // ждём SHORT
            if (evt == BTN_EVT_SHORT_PRESS) 
            {
                // Пуск: 3 вспышки по 250/250 мс.
                led_blink(&g_led_green, CS_BLINK_COUNT, CS_BLINK_MS, CS_BLINK_MS);
                s_state = CS_ST_LAUNCHING;
            }
            break;

        case CS_ST_LAUNCHING: // ждём, пока led_blink сам не завершит серию
            if (!led_is_blinking(&g_led_green)) 
            {
                led_off(&g_led_green);
                s_state = CS_ST_IDLE;
            }
            break;
    }
}

uint8_t combat_system_is_active(void)       // Возвращает 1, если FSM не в IDLE, иначе 0.
{
    return (s_state != CS_ST_IDLE) ? 1u : 0u;
}

cs_state_t combat_system_get_state(void)    // Возвращает текущее состояние FSM боевого режима.
{
    return s_state;
}