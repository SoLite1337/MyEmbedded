#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// Настройка пинов
#define POT_PORT    GPIOA
#define POT_PIN     GPIO_PIN_0          // A0 = ADC1_IN0
#define LED_PORT    GPIOA
#define LED_PIN     GPIO_PIN_1          // A1 = TIM2_CH2 (AF1)
#define MOTOR_PORT  GPIOA
#define MOTOR_PIN   GPIO_PIN_3          // A3 = TIM5_CH4 (AF2)

// ================= TIM2 = LED, 1 кГц =================
#define LED_PSC     15U                              /* 16 МГц / 16 = 1 МГц */
#define LED_ARR     (1000000U / 1000U - 1U)          /* 999                  */
#define LED_STEPS   (LED_ARR + 1U)                   /* 1000 рівнів          */

// ================= TIM5 = Motor, 20 кГц =================
#define MOTOR_PSC   0U
#define MOTOR_ARR   (16000000U / 20000U - 1U)        /* 799                  */
#define MOTOR_STEPS (MOTOR_ARR + 1U)                 /* 800 рівнів           */

// ================= TIM3 = тригер ADC, 1 кГц =================
#define ADC_TRIG_PSC 15U
#define ADC_TRIG_ARR (1000000U / 1000U - 1U)         /* 999                  */

#define ADC_MAX     4095U
#define PRINT_EVERY 500U

// ================= Глобальные переменные =================
ADC_HandleTypeDef  hadc1;
TIM_HandleTypeDef  htim2;
TIM_HandleTypeDef  htim3;
TIM_HandleTypeDef  htim5;
UART_HandleTypeDef huart2;

volatile uint16_t g_raw   = 0;
volatile uint32_t g_led   = 0;
volatile uint32_t g_motor = 0;
volatile bool     g_new   = false;

void Error_Handler(void);

// Масштабирование
static uint32_t scale(uint16_t raw, uint32_t steps)
{
    if (raw > ADC_MAX) raw = ADC_MAX;
    return (uint32_t)raw * steps / (ADC_MAX + 1U);
}

// Системный такт 16 МГц (HSI) 
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState        = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();

    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                       | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0) != HAL_OK) Error_Handler();
}

// Инциализация GPIO
static void GPIO_Init(void)
{
    GPIO_InitTypeDef g = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* A1 = LED, TIM2_CH2 (AF1) */
    g.Pin       = LED_PIN;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_LOW;
    g.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(LED_PORT, &g);

    /* A3 = Motor, TIM5_CH4 (AF2) */
    g.Pin       = MOTOR_PIN;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_LOW;
    g.Alternate = GPIO_AF2_TIM5;
    HAL_GPIO_Init(MOTOR_PORT, &g);

    /* A2 = USART2_TX (AF7) */
    g.Pin       = GPIO_PIN_2;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_PULLUP;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &g);

    /* A0 = ADC input */
    g.Pin       = POT_PIN;
    g.Mode      = GPIO_MODE_ANALOG;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_LOW;
    g.Alternate = 0;
    HAL_GPIO_Init(POT_PORT, &g);
}

// Инциализация UART2
static void UART2_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();

    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

// Инциализация ADC1
static void ADC_Init(void)
{
    ADC_ChannelConfTypeDef ch = {0};

    __HAL_RCC_ADC1_CLK_ENABLE();

    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T3_TRGO;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);

    ch.Channel      = ADC_CHANNEL_0;
    ch.Rank         = 1;
    ch.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &ch);

    HAL_NVIC_SetPriority(ADC_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(ADC_IRQn);
}

// TIM2 для LED PWM 
static void TIM2_Init(void)
{
    TIM_OC_InitTypeDef oc = {0};

    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = LED_PSC;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = LED_ARR;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim2);

    oc.OCMode     = TIM_OCMODE_PWM1;
    oc.Pulse      = 0;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &oc, TIM_CHANNEL_2);

    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

// TIM3 для триггера ADC
static void TIM3_Init(void)
{
    TIM_MasterConfigTypeDef m = {0};

    __HAL_RCC_TIM3_CLK_ENABLE();

    htim3.Instance               = TIM3;
    htim3.Init.Prescaler         = ADC_TRIG_PSC;
    htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim3.Init.Period            = ADC_TRIG_ARR;
    htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim3);

    m.MasterOutputTrigger = TIM_TRGO_UPDATE;
    m.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim3, &m);

    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start(&htim3);
}

// TIM5 для мотора PWM
static void TIM5_Init(void)
{
    TIM_OC_InitTypeDef oc = {0};

    __HAL_RCC_TIM5_CLK_ENABLE();

    htim5.Instance               = TIM5;
    htim5.Init.Prescaler         = MOTOR_PSC;
    htim5.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim5.Init.Period            = MOTOR_ARR;
    htim5.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim5);

    oc.OCMode     = TIM_OCMODE_PWM1;
    oc.Pulse      = 0;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim5, &oc, TIM_CHANNEL_4);

    __HAL_TIM_CLEAR_FLAG(&htim5, TIM_FLAG_UPDATE);
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_4);
}

// Логика главного цикла 
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART2_Init();
    TIM2_Init();
    TIM3_Init();
    TIM5_Init();
    ADC_Init();

    setvbuf(stdout, NULL, _IONBF, 0);

    HAL_ADC_Start_IT(&hadc1);

    for (;;)
    {
        if (g_new)
        {
            uint16_t raw   = g_raw;
            uint32_t led   = g_led;
            uint32_t motor = g_motor;
            uint32_t led_pct;
            uint32_t motor_pct;

            g_new = false;

            led_pct   = led   * 100u / LED_STEPS;
            motor_pct = motor * 100u / MOTOR_STEPS;

            printf("pot=%4u  LED=%3u%%  MOTOR=%3u%%\r\n",
                   (unsigned)raw,
                   (unsigned)led_pct,
                   (unsigned)motor_pct);
        }
    }
}

// Обротчики прерываний 
void SysTick_Handler(void) 
{ 
    HAL_IncTick(); 
}
void ADC_IRQHandler(void)  
{ 
    HAL_ADC_IRQHandler(&hadc1); 
}

// Переопределение функции _write для printf
int _write(int fd, char *ptr, int len)
{
    (void)fd;
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);
    return len;
}

// Callback ADC
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    static uint16_t cnt = 0;

    if (hadc->Instance != ADC1)
        return;

    uint16_t raw   = (uint16_t)HAL_ADC_GetValue(hadc);
    uint32_t led   = scale(raw, LED_STEPS);
    uint32_t motor = scale(raw, MOTOR_STEPS);

    // Установка значений ШИМ
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, led);
    __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_4, motor);

    g_raw   = raw;
    g_led   = led;
    g_motor = motor;

    if (++cnt >= PRINT_EVERY) { cnt = 0; g_new = true; }
}

// /* ================= Error ================= */
void Error_Handler(void)
{
    __disable_irq();
    for (;;) { }
}