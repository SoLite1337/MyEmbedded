#define CONFIG_H
#include "stm32f4xx_hal.h"

#define LED_RED_PORT     GPIOA
#define LED_RED_PIN      GPIO_PIN_0
#define LED_GREEN_PORT   GPIOA
#define LED_GREEN_PIN    GPIO_PIN_1
#define LED_YELLOW_PORT  GPIOA
#define LED_YELLOW_PIN   GPIO_PIN_2

#define BTN_THERMAL_PORT GPIOA
#define BTN_THERMAL_PIN  GPIO_PIN_3
#define BTN_COMBAT_PORT  GPIOA
#define BTN_COMBAT_PIN   GPIO_PIN_4
#define BTN_SENSOR_PORT  GPIOA
#define BTN_SENSOR_PIN   GPIO_PIN_5


// Инициализация конфигурации (кнопки, светодиоды)
void config_init(void);
uint8_t config_button_pressed(GPIO_TypeDef *port, uint16_t pin);
void config_led_write(GPIO_TypeDef *port, uint16_t pin, uint8_t on);