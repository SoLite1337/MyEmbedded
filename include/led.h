#define LED_H
#include <stdint.h>
#include "stm32f4xx_hal.h"

// НАЗНАЧЕНИЕ:
//   Драйвер светодиода:
//     - постоянное включение (led_on);
//     - постоянное выключения (led_off);
//     - серии миганий (led_blink) с указанием количества вспышек,
//       времени ON и времени OFF.
//     - Реализован как FSM с состояниями led_state_t.


#define LED_BLINK_FOREVER   0u   // постоянное мигание    

// Состояния FSM светодиода.
typedef enum 
{
    LED_ST_OFF,        // постоянно выключен
    LED_ST_ON,         // постоянно включён
    LED_ST_BLINK_ON,   // фаза свечения мигания
    LED_ST_BLINK_OFF,  // фаза паузы мигания
} led_state_t;

// Структура для управления светодиодом.
typedef struct 
{
    GPIO_TypeDef *port;         // порт
    uint16_t      pin;          // пин
    led_state_t   state;        // состояние
    uint32_t      timer;        // сколько мс осталось в текущей фазе
    uint32_t      on_ms;        // длительность фазы «ON»
    uint32_t      off_ms;       // длительность фазы «OFF»
    uint32_t      blinks_left;  // сколько вспышек осталось
    uint8_t       infinite;     // 1 - мигать постояннно
} led_t;




void led_init(led_t *l, GPIO_TypeDef *port, uint16_t pin, uint8_t active_low); // инициализация структуры
void led_on(led_t *l); // включить светодиод
void led_off(led_t *l); // выключить светодиод
void led_blink(led_t *l, uint32_t count, uint32_t on_ms, uint32_t off_ms); // начать мигание
void led_tick(led_t *l); // раз в 1 мс
uint8_t led_is_blinking(led_t *l); // проверяем LED на мигание
led_state_t led_get_state(const led_t *l); // получить текущее состояние светодиода  