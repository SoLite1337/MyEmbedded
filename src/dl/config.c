#include "dl/config.h"

// ==================================================================
// config.c — реалізація шару конфігурації заліза (Data Layer).
// - Ініціалізація GPIO, EXTI, NVIC, TIM3.
// - Тонкі обгортки над HAL.
// ==================================================================

TIM_HandleTypeDef htim3;   // Визначено тут — extern у main.c

// Настройка пінів: LED як виходи, кнопки як EXTI-входи з pull-up.
static void config_gpio_init(void)
{
    GPIO_InitTypeDef g = {0};

    // Світлодіоди: push-pull виходи
    g.Pin   = LED_RED_PIN | LED_GREEN_PIN | LED_YELLOW_PIN;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &g);

    // Кнопки: переривання по обох фронтах, внутрішній pull-up
    g.Pin   = BTN_THERMAL_PIN | BTN_COMBAT_PIN | BTN_SENSOR_PIN;
    g.Mode  = GPIO_MODE_IT_RISING_FALLING;
    g.Pull  = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &g);
}

// Дозвіл переривань EXTI для PA3, PA4, PA5 з пріоритетом 2
static void config_exti_nvic_init(void)
{
    HAL_NVIC_SetPriority(EXTI3_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);
    HAL_NVIC_SetPriority(EXTI4_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

// TIM3: HSI 16 МГц / 16 = 1 МГц -> період 1000 -> переривання кожну 1 мс.
// Пріоритет ВИЩЕ (1), ніж у EXTI (2), щоб тік не губився.
static void config_tim3_init(void)
{
    htim3.Instance               = TIM3;
    htim3.Init.Prescaler         = 16 - 1;
    htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim3.Init.Period            = 1000 - 1;
    htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim3);

    HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);

    HAL_TIM_Base_Start_IT(&htim3);
}

// Ініціалізація периферії плати та запуск TIM3 (тік 1 мс).
void config_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();   // потрібно для EXTI
    __HAL_RCC_TIM3_CLK_ENABLE();

    config_gpio_init();
    config_exti_nvic_init();
    config_tim3_init();
}

// Прочитати «сире» стан кнопки: 1 - натиснута (LOW), 0 - відпущена (HIGH).
uint8_t config_button_pressed(GPIO_TypeDef *port, uint16_t pin)
{
    return (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) ? 1u : 0u;
}

// Керування світлодіодом
void config_led_write(GPIO_TypeDef *port, uint16_t pin, uint8_t on)
{
    HAL_GPIO_WritePin(port, pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}