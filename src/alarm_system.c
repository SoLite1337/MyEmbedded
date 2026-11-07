#include "alarm_system.h"
#include "led.h"
#include "button.h"
#include "thermal_traps.h"

extern button_t g_btn_sensor;
extern led_t    g_led_yellow;
static as_state_t s_state; //текущее состояние автомата

void alarm_system_init(void) //Инициализация FSM. Ставим автомат в исходное состояние IDLE.
{ 
    s_state = AS_ST_IDLE; 
}



// Один шаг FSM. Вызывается раз в 1 мс из app_tick() (TIM3 ISR).
//
// На каждом шаге:
//   1) забираем ОДНО накопившееся событие кнопки (если есть);
//
//   2) проверяем, не сообщили ли тепловые ловушки о завершении серии (burst_done);
//
//   3) в зависимости от текущего состояния и входов выполняем переход и/или действие над LED.
//

void alarm_system_tick(void) 
{
    btn_event_t evt = button_get_event(&g_btn_sensor); 
    uint8_t burst_done = thermal_traps_consume_burst_done();

    // FSM автомата подсистемы тревоги

    switch (s_state) 
    {
        case AS_ST_IDLE:

            if (evt == BTN_EVT_SHORT_PRESS) 
            {
                led_on(&g_led_yellow);  // индикация предупреждения
                s_state = AS_ST_WARNING; // устанавливаем состояние WARNING
            } 
            else if (evt == BTN_EVT_LONG_PRESS) 
            {
                led_blink(&g_led_yellow, LED_BLINK_FOREVER, AS_BLINK_MS, AS_BLINK_MS); // индикация тревоги
                s_state = AS_ST_ALARM; // устанавливаем состояние ALARM
            }
            break;

        case AS_ST_WARNING: //обрабатываем события в состоянии WARNING
            if (evt == BTN_EVT_LONG_PRESS) 
            {
                led_blink(&g_led_yellow, LED_BLINK_FOREVER, AS_BLINK_MS, AS_BLINK_MS);
                s_state = AS_ST_ALARM;
            } 
            else if (evt == BTN_EVT_SHORT_PRESS) 
            {
                led_off(&g_led_yellow);
                s_state = AS_ST_IDLE;
            } 
            else if (burst_done) 
            {
                // Тепловые ловушки также гасят предупреждение и аларм
                led_off(&g_led_yellow);
                s_state = AS_ST_IDLE;
            }
            break;

        case AS_ST_ALARM:
            if (burst_done) 
            {
                // Тепловые ловушки также гасят предупреждение и аларм
                led_off(&g_led_yellow);
                s_state = AS_ST_IDLE;
            }
            break;
    }
}

uint8_t alarm_system_is_active(void)
{
    return (s_state != AS_ST_IDLE) ? 1u : 0u;
}

as_state_t alarm_system_get_state(void)
{
    return s_state;
}