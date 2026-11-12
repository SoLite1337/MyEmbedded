#define LED_H
#include <stdint.h>
#include "stm32f4xx_hal.h"

#define LED_BLINK_FOREVER   0u

// Состояния светодиода
typedef enum 
{
    LED_ST_OFF, 
    LED_ST_ON, 
    LED_ST_BLINK_ON, 
    LED_ST_BLINK_OFF,
} led_state_t;

// Структура для управления светодиодом
typedef struct 
{
    GPIO_TypeDef *port; uint16_t pin;
    led_state_t state; uint32_t timer;
    uint32_t on_ms; uint32_t off_ms;
    uint32_t blinks_left; uint8_t infinite;
} led_t;

// Инициализация светодиода
void led_init(led_t *l, GPIO_TypeDef *port, uint16_t pin, uint8_t active_low);
void led_on(led_t *l);
void led_off(led_t *l);
void led_blink(led_t *l, uint32_t count, uint32_t on_ms, uint32_t off_ms);
void led_tick(led_t *l);
uint8_t led_is_blinking(led_t *l);
led_state_t led_get_state(const led_t *l);