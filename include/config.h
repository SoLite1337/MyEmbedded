#define CONFIG_H
#include "stm32f4xx_hal.h"

// НАЗНАЧЕНИЕ:
//     1. Макросы пинов, портов и их режимов.
//     2. Инициализация периферии: GPIO, EXTI, NVIC, TIM3.
//

// Светодиоды: active-high
#define LED_RED_PORT     GPIOA       // Красный: система тепловых ловушек
#define LED_RED_PIN      GPIO_PIN_0
#define LED_GREEN_PORT   GPIOA       // Зелёный: боевой режим
#define LED_GREEN_PIN    GPIO_PIN_1
#define LED_YELLOW_PORT  GPIOA       // Жёлтый: датчик облучения / тревога
#define LED_YELLOW_PIN   GPIO_PIN_2

// Кнопки: active-low, внутренний pull-up, EXTI по обоим фронтам
#define BTN_THERMAL_PORT GPIOA       // Пуск тепловых ловушек
#define BTN_THERMAL_PIN  GPIO_PIN_3
#define BTN_COMBAT_PORT  GPIOA       // Боевой режим
#define BTN_COMBAT_PIN   GPIO_PIN_4
#define BTN_SENSOR_PORT  GPIOA       // Датчик облучения
#define BTN_SENSOR_PIN   GPIO_PIN_5



// Инициализирует всю периферию платы и запускает TIM3 (тик 1 мс). */
void config_init(void);

// Прочитать «сырое» состояние кнопки.
uint8_t config_button_pressed(GPIO_TypeDef *port, uint16_t pin);

// Управление светодиодом.
void config_led_write(GPIO_TypeDef *port, uint16_t pin, uint8_t on);