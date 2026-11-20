#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ==================== ВЫВОДЫ МК ====================
 * PA1 - выход ШИМ для сервопривода (TIM2_CH2)
 * PA0 - вход АЦП от потенциометра (ADC1_IN0)
 * PA2 - TX USART2
 * =================================================== */

#define SERVO_PORT GPIOA
#define SERVO_PIN  GPIO_PIN_1
#define POT_PORT   GPIOA
#define POT_PIN    GPIO_PIN_0

/* ==================== ШИМ сервопривода ====================
 * Частота 50 Гц (период 20 мс), длительность импульса задаётся в микросекундах.
 * Таймер тактируется от APB1 = 16 МГц (HSI по умолчанию, см. SystemClock_Config).
 * 16 МГц / PSC(16) = 1 МГц → 1 тик = 1 мкс.
 * ======================================================== */
#define SERVO_FREQ_HZ   50U
#define TIM_TICK_HZ     1000000U
#define PWM_PSC         16U
#define PWM_ARR         (TIM_TICK_HZ / SERVO_FREQ_HZ - 1U)  // 20000 - 1 = период 20 мс

#define SERVO_MIN_US    500U    // импульс для 0°   
#define SERVO_MAX_US    2400U   // импульс для 180° 
#define SERVO_MAX_ANGLE 180U    // максимальный угол сервопривода

/* ==================== АЦП / сглаживание ==================== */
#define ADC_MAX      4095U
#define ADC_SAMPLES  8U     // количество отсчётов для усреднения
#define ANGLE_HYST   1      // антидребезг: обновлять только при изменении на >= 1°

/* ==================== Глобальные объекты HAL ==================== */
TIM_HandleTypeDef  htim2;
UART_HandleTypeDef huart2;
ADC_HandleTypeDef  hadc1;

/* ==================== Инициализация GPIO ==================== */
static void GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};

    /* PA1 - ШИМ на TIM2_CH2 */
    g.Pin       = SERVO_PIN;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_LOW;
    g.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(SERVO_PORT, &g);

    /* PA2 - TX USART2 */
    g.Pin       = GPIO_PIN_2;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_PULLUP;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &g);

    /* PA0 - аналоговый вход (потенциометр) */
    g.Pin  = POT_PIN;
    g.Mode = GPIO_MODE_ANALOG;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(POT_PORT, &g);
}

/* ==================== USART2 (только TX) ==================== */
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

/* ==================== TIM2 в режиме ШИМ ==================== */
static void TIM2_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = PWM_PSC - 1;         // 1 МГц тик
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = PWM_ARR;             // 20 мс (50 Гц)
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim2);

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode     = TIM_OCMODE_PWM1;
    oc.Pulse      = (SERVO_MIN_US + SERVO_MAX_US) / 2U; // старт по центру
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &oc, TIM_CHANNEL_2);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

/* ==================== ADC1 (PA0, 12 бит, одиночные) ==================== */
static void ADC1_Init(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();

    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = ADC_CHANNEL_0;                    // PA0
    ch.Rank         = 1;
    ch.SamplingTime = ADC_SAMPLETIME_84CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &ch);

}

/* ==================== Усреднённое чтение АЦП ====================
 * Одна пара Start/Stop на весь пакет. Каждое одиночное преобразование
 * запускается вручную через бит SWSTART — это быстрее, чем
 * вызывать HAL_ADC_Start на каждый отсчёт.
 * ============================================================== */
static uint32_t adc_read_avg(void)
{
    uint32_t sum = 0;

    HAL_ADC_Start(&hadc1);              // включаем АЦП один раз

    for (uint32_t i = 0; i < ADC_SAMPLES; i++)
    {
        ADC1->CR2 |= ADC_CR2_SWSTART;        // ручной запуск одиночного преобразования
        while (!(ADC1->SR & ADC_SR_EOC)) { } // ждём флаг End Of Conversion
        sum += ADC1->DR;                     // чтение DR автоматически сбрасывает EOC
    }

    HAL_ADC_Stop(&hadc1);
    return sum / ADC_SAMPLES;
}

/* ==================== Установка угла сервопривода (0..180°) ==================== */
static void set_servo_angle(uint32_t angle)
{
    if (angle > SERVO_MAX_ANGLE)
    {
        angle = SERVO_MAX_ANGLE;
    }

    /* Линейная интерполяция: 0° → SERVO_MIN_US, 180° → SERVO_MAX_US */
    uint32_t pulse = SERVO_MIN_US + ((SERVO_MAX_US - SERVO_MIN_US) * angle) / SERVO_MAX_ANGLE;

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, pulse);
}

void Error_Handler(void); /* forward */

/* ==================== main ==================== */
int main(void)
{
    HAL_Init();
    GPIO_Init();
    UART2_Init();
    TIM2_Init();
    ADC1_Init();

    /* Отключаем буферизацию stdout - printf сразу уходит в UART через _write */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("Servo control started\r\n");

    int last_angle = -100; /* заведомо недопустимое значение - форсируем первый апдейт */

    while (1)
    {
        uint32_t raw = adc_read_avg();

        /* Перевод отсчёта АЦП в градусы: angle / 180 = raw / 4095 */
        int angle = (int)((raw * SERVO_MAX_ANGLE) / ADC_MAX);

        if (abs(angle - last_angle) >= ANGLE_HYST)
        {
            last_angle = angle;
            set_servo_angle((uint32_t)angle);
            printf("ADC=%4lu  angle=%3d deg\r\n", (unsigned long)raw, angle);
        }

        HAL_Delay(20); /* один период сервопривода (20 мс) */
    }
}

/* ==================== Обработчики  ==================== */
#ifdef __cplusplus
extern "C" {
#endif

void SysTick_Handler(void)
{
    HAL_IncTick();
}

// Ретаргет printf → UART. newlib вызывает _write для каждого printf
int _write(int fd, char *ptr, int len)
{
    (void)fd;
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);
    return len;
}

#ifdef __cplusplus
}
#endif

void Error_Handler(void)
{
    __disable_irq();
    while (1) { }
}