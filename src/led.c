#include "led.h"
#include "config.h"


// НАЗНАЧЕНИЕ:
//   Драйвер светодиода:
//     - постоянное включение (led_on);
//     - постоянное выключения (led_off);
//     - серии миганий (led_blink) с указанием количества вспышек, времени ON и времени OFF.
//     - Реализован как FSM с состояниями led_state_t.


//Инициализация структуры
void led_init(led_t *l, GPIO_TypeDef *port, uint16_t pin, uint8_t active_low)
{
    (void)active_low;   /* наши LED active-high — параметр для переноса */
    l->port        = port;
    l->pin         = pin;
    l->state       = LED_ST_OFF;
    l->timer       = 0;
    l->on_ms       = 0;
    l->off_ms      = 0;
    l->blinks_left = 0;
    l->infinite    = 0;
    config_led_write(port, pin, 0);
}

// Включить светодиод и сбросить все параметры мигания
void led_on(led_t *l)
{
    l->state       = LED_ST_ON;
    l->blinks_left = 0;
    l->infinite    = 0;
    config_led_write(l->port, l->pin, 1);
}

// Выключить светодиод и сбросить все параметры мигания
void led_off(led_t *l)
{
    l->state       = LED_ST_OFF;
    l->blinks_left = 0;
    l->infinite    = 0;
    config_led_write(l->port, l->pin, 0);
}

// Запустить серию миганий. count=LED_BLINK_FOREVER
void led_blink(led_t *l, uint32_t count, uint32_t on_ms, uint32_t off_ms)
{
    l->on_ms       = on_ms;
    l->off_ms      = off_ms;
    l->infinite    = (count == LED_BLINK_FOREVER) ? 1u : 0u;
    l->blinks_left = count;
    l->timer       = on_ms;
    l->state       = LED_ST_BLINK_ON;
    config_led_write(l->port, l->pin, 1);
}

void led_tick(led_t *l)
{
    switch (l->state) 
    {
        case LED_ST_OFF:
            break;   /* статика - ничего не делаем */

        case LED_ST_ON:
            break;   /* статика - ничего не делаем */

        case LED_ST_BLINK_ON:
            // Фаза  мигания. Ждём окончания таймера.
            if (l->timer > 0) 
            {
                l->timer--;
            }
            // Если таймер закончился, то выключаем светодиод и переходим в фазу паузы.
            if (l->timer == 0) 
            {
                config_led_write(l->port, l->pin, 0);
                l->state = LED_ST_BLINK_OFF;
                l->timer = l->off_ms;
                if (!l->infinite) 
                // Если мигание не бесконечное, то уменьшаем счётчик вспышек.
                {
                    if (l->blinks_left > 0) 
                    {
                        l->blinks_left--;
                    }
                    if (l->blinks_left == 0) 
                    {
                        l->state = LED_ST_OFF;
                    }
                }
            }
            break;

        case LED_ST_BLINK_OFF:
            // Фаза паузы. Ждём окончания таймера.
            if (l->timer > 0) 
            {
                l->timer--;
            }
            if (l->timer == 0) 
            {
                // Пауза закончилась, включаем, новая вспышка.
                config_led_write(l->port, l->pin, 1);
                l->state = LED_ST_BLINK_ON;
                l->timer = l->on_ms;
            }
            break;
    }
}


// Проверяем LED на мигание
uint8_t led_is_blinking(led_t *l)
{
    return (l->state == LED_ST_BLINK_ON || l->state == LED_ST_BLINK_OFF) ? 1u : 0u;
}


// Получить текущее состояние светодиода
led_state_t led_get_state(const led_t *l)
{
    return l->state;
}