#include "config.h"

// НАЗНАЧЕНИЕ: 
//     config.c — реализация слоя конфигурации платы.
//       - Инициализация GPIO, EXTI, NVIC, TIM3.
//       - Обертки над HAL для доступа к железу.

TIM_HandleTypeDef htim3;   // Определён здесь - extern в main.c





// Настройка пинов: LED как выходы, кнопки как EXTI-входы с pull-up.
static void config_gpio_init(void)
{
    GPIO_InitTypeDef g = {0};

    // Светодиоды:
    g.Pin   = LED_RED_PIN | LED_GREEN_PIN | LED_YELLOW_PIN;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &g);

    // Кнопки: 
    g.Pin   = BTN_THERMAL_PIN | BTN_COMBAT_PIN | BTN_SENSOR_PIN;
    g.Mode  = GPIO_MODE_IT_RISING_FALLING;
    g.Pull  = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &g);
}

// Разрешение прерываний EXTI для PA3, PA4, PA5 с приоритетом 2 (ниже таймера).
static void config_exti_nvic_init(void)
{
    HAL_NVIC_SetPriority(EXTI3_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);
    HAL_NVIC_SetPriority(EXTI4_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

// TIM3: HSI 16 МГц / 16 = 1 МГц -> период 1000 -> прерывание каждую 1 мс.
// Приоритет ВЫШЕ, чем у EXTI, чтобы тик не терял кнопки. */
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


// Инициализация периферии платы и запуск TIM3 (тик 1 мс).
void config_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();   /* нужно для EXTI */
    __HAL_RCC_TIM3_CLK_ENABLE();

    config_gpio_init();
    config_exti_nvic_init();
    config_tim3_init();
}

// Прочитать «сырое» состояние кнопки: 1 - нажата (LOW), 0 - отпущена (HIGH через pull-up)
uint8_t config_button_pressed(GPIO_TypeDef *port, uint16_t pin)
{
    /* Кнопка замыкает пин на GND — нажата означает RESET. */
    return (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) ? 1u : 0u;
}

// Управление светодиодом
void config_led_write(GPIO_TypeDef *port, uint16_t pin, uint8_t on)
{
    HAL_GPIO_WritePin(port, pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}