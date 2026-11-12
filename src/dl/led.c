#include "dl/led.h"
#include "dl/config.h"

// ==================================================================
// Реализация управления светодиодами.
// ==================================================================

// Инициализация светодиода, назначение пина и активного уровня
void led_init(led_t *l, GPIO_TypeDef *port, uint16_t pin, uint8_t active_low)
{
    (void)active_low;   /* наші LED active-high */
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

// Включение светодиода
void led_on(led_t *l)
{
    l->state       = LED_ST_ON;
    l->blinks_left = 0;
    l->infinite    = 0;
    config_led_write(l->port, l->pin, 1);
}

// Выключение светодиода
void led_off(led_t *l)
{
    l->state       = LED_ST_OFF;
    l->blinks_left = 0;
    l->infinite    = 0;
    config_led_write(l->port, l->pin, 0);
}

// Моргание светодиодом: count раз, on_ms миллисекунд включен, off_ms миллисекунд выключен
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

// Тик таймера для светодиода, должен вызываться каждые 1 мс
void led_tick(led_t *l)
{
    switch (l->state) 
    {
        case LED_ST_OFF:

            break;   /* статика — нічого не робимо */
        case LED_ST_ON:
            break;   /* статика — нічого не робимо */

        case LED_ST_BLINK_ON:
            if (l->timer > 0) 
            {
                l->timer--;
            }
            if (l->timer == 0) 
            {
                config_led_write(l->port, l->pin, 0);
                l->state = LED_ST_BLINK_OFF;
                l->timer = l->off_ms;
                if (!l->infinite) 
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
            if (l->timer > 0) 
            {
                l->timer--;
            }
            if (l->timer == 0) 
            {
                config_led_write(l->port, l->pin, 1);
                l->state = LED_ST_BLINK_ON;
                l->timer = l->on_ms;
            }
            break;
    }
}

uint8_t led_is_blinking(led_t *l)
{
    return (l->state == LED_ST_BLINK_ON || l->state == LED_ST_BLINK_OFF) ? 1u : 0u;
}

led_state_t led_get_state(const led_t *l)
{
    return l->state;
}