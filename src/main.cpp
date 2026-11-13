#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdio.h>

#define GPIO_PORT GPIOA
#define LED_PIN GPIO_PIN_1
#define LDR_PIN GPIO_PIN_0
#define LED_ACTIVE_HIGH 1

#define TH_DARK  2100  // отфильтрованное значение ниже этого - темно, LED включен
#define TH_LIGHT 2700  // отфильтрованное значение выше этого - светло, LED выключен

// ---------- настройки SMA ----------
#define SMA_SIZE 16    // размер окна скользящего среднего

// ---------- темп вывода в UART ----------
#define PRINT_PERIOD_TICKS 50   // 50 Гц / 50 = 1 Гц

// ---------- глобальные переменные----------
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart2;

volatile uint16_t g_raw        = 0;      // сырое значение с ADC
volatile uint16_t g_filtered   = 0;      // значение после SMA
volatile bool     g_print_tick = false;  // флаг: пора печатать в UART
volatile bool     g_dark       = false;  // текущее состояние "темно/светло"

// ---------- состояние SMA (исп. из ISR АЦП) ----------
static uint16_t sma_buf[SMA_SIZE] = {0}; // кольцевой буфер отсчётов
static uint32_t sma_sum = 0;             // текущая сумма всех элементов буфера
static uint8_t  sma_idx = 0;             // индекс записи в кольцевой буфер
static uint8_t  sma_cnt = 0;             // сколько отсчётов уже накоплено

// ---------- делитель тиков TIM2 ----------
static uint32_t tim2_tick_cnt = 0;       // счётчик тиков для печати раз в секунду

// ---------- вспомогательные функции ----------
static void led_set(int on)
{
    GPIO_PinState s = (on == LED_ACTIVE_HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(GPIO_PORT, LED_PIN, s);
}

// Обновляет SMA новым отсчётом и возвращает текущее среднее
static uint16_t sma_update(uint16_t value)
{
    sma_sum -= sma_buf[sma_idx];                    // вычитаем самый старый отсчёт
    sma_buf[sma_idx] = value;                       // записываем новый на его место
    sma_sum += value;                               // добавляем новый к сумме
    sma_idx = (uint8_t)((sma_idx + 1) % SMA_SIZE);
    if (sma_cnt < SMA_SIZE) 
    {
        sma_cnt++;                                  // пока буфер не заполнен — считаем сколько есть
    }
    return (uint16_t)(sma_sum / sma_cnt);           // делим на реальное количество (корректно при "прогреве")
}

static void GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};

    g.Pin = LED_PIN;                          // LED — выход
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_PORT, &g);
    led_set(0);

    g.Pin = GPIO_PIN_2;                       // PA2 — TX USART2
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIO_PORT, &g);

    g.Pin = LDR_PIN;                          // PA0 — вход АЦП
    g.Mode = GPIO_MODE_ANALOG;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_PORT, &g);
}

// ---------- инициализация периферии ----------
static void UART2_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

// ---------- инициализация АЦП ----------
static void ADC_Init(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;                 // oneshot режим
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel = ADC_CHANNEL_0;                              // канал 0 — PA0
    ch.Rank = 1;
    ch.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &ch);

    HAL_NVIC_SetPriority(ADC_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(ADC_IRQn);
}


// ---------- инициализация TIM2 ----------
static void TIM2_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 16000 - 1;        // 16 МГц / 16000 = 1 кГц
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 20 - 1;              // 1 кГц / 20 = 50 Гц
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim2);

    HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start_IT(&htim2);
}

void Error_Handler(void);

int main(void)
{
    HAL_Init();
    GPIO_Init();
    UART2_Init();
    ADC_Init();
    TIM2_Init();

    while (1)
    {
        if (g_print_tick)                     // печатаем только раз в секунду
        {
            printf("raw %u  filtered %u  dark=%d\n", (unsigned)g_raw, (unsigned)g_filtered, (int)g_dark);
            g_print_tick = false;
        }
    }
}

#ifdef __cplusplus
extern "C"
{
#endif

    // ---------------- обработчики прерываний ----------------

    void SysTick_Handler(void)
    {
        HAL_IncTick();
    }

    int _write(int fd, char *ptr, int len)
    {
        (void)fd;
        HAL_UART_Transmit(&huart2, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);
        return len;
    }

    void TIM2_IRQHandler(void)
    {
        HAL_TIM_IRQHandler(&htim2);
    }

    void ADC_IRQHandler(void)
    {
        HAL_ADC_IRQHandler(&hadc1);           // вызывает HAL_ADC_ConvCpltCallback
    }

    // ---------------- колбэки ----------------

    void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
    {
        if (TIM2 == htim->Instance)
        {
            // раз в секунду выставляем флаг для printf в main()
            if (++tim2_tick_cnt >= PRINT_PERIOD_TICKS)
            {
                tim2_tick_cnt = 0;
                g_print_tick = true;
            }

            HAL_ADC_Start_IT(&hadc1);         // запускаем АЦП каждый тик (50 Гц)
        }
    }

    // Колбэк завершения преобразования АЦП
    void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
    {
        if (ADC1 == hadc->Instance)
        {
            g_raw = (uint16_t)HAL_ADC_GetValue(hadc);

            // обновляем SMA и сохраняем результат
            uint16_t filtered = sma_update(g_raw);
            g_filtered = filtered;

            // гистерезис уже по отфильтрованному значению
            if (!g_dark && filtered < TH_DARK)
            {
                g_dark = true;
                led_set(1);
            }
            else if (g_dark && filtered > TH_LIGHT)
            {
                g_dark = false;
                led_set(0);
            }
        }
    }

#ifdef __cplusplus
}
#endif

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}