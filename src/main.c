#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdio.h>
#include <math.h> 

// --- Определение пинов и портов ---
#define GPIO_PORT GPIOA
#define LED_PIN   GPIO_PIN_1    // Пин PA1 для светодиода
#define LDR_PIN   GPIO_PIN_0    // Пин PA0 для потенциометра (АЦП)
#define LED_ACTIVE_HIGH 1       // 1 - светодиод загорается при высоком уровне, 0 - при низком


// Гистерезис
// Светодиод включается, если значение падает ниже 2100,
// и выключается, только если значение поднимется выше 2700.
#define TH_DARK  2100
#define TH_LIGHT 2700


// Периоды работы таймеров и вывода
#define ADC_PERIOD_MS   100U  // Опрос датчика каждые 100 мс (10 Гц)
#define VREF_PERIOD_MS  100U  // Калибровка опорного напряжения каждые 100 мс
#define PRINT_PERIOD_MS 2000U // Вывод таблицы в консоль раз в 2 секунды


// Параметры для калибровки внутреннего опорного напряжения (VREFINT)
#define VDDA_NOMINAL_MV 3300U // Номинальное напряжение питания (3.3 В)
#define VREFINT_AVG_N   8     // Количество усредняемых замеров VREFINT для сглаживания



// Глобальные переменные (разделяемые с прерываниями)
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim5;
UART_HandleTypeDef huart2;


volatile uint16_t g_raw = 0;                      // Сырое значение с АЦП
volatile bool     g_new = false;                  // Флаг: пришли новые данные
volatile bool     g_dark = false;                 // Флаг: текущее состояние освещенности
volatile uint16_t g_vdda_mv = VDDA_NOMINAL_MV;    // Измеренное напряжение питания VDDA (в мВ)


// Функция управления светодиодом
static void led_set(int on)
{
    GPIO_PinState s = (on == LED_ACTIVE_HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(GPIO_PORT, LED_PIN, s);
}

// Инициализация пинов ввода-вывода
static void GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE(); // Включаем тактирование порта A

    GPIO_InitTypeDef g = {0};

    // Настройка пина светодиода (PA1)
    g.Pin = LED_PIN; 
    g.Mode = GPIO_MODE_OUTPUT_PP; // Выход, push-pull
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_PORT, &g);
    led_set(0); // Изначально выключаем светодиод


    // Настройка пина UART (PA2 - TX)
    g.Pin = GPIO_PIN_2; 
    g.Mode = GPIO_MODE_AF_PP; // Альтернативная функция, push-pull
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART2; // Привязка к USART2
    HAL_GPIO_Init(GPIO_PORT, &g);

    // Настройка пина АЦП для потенциометра (PA0)
    g.Pin = LDR_PIN; 
    g.Mode = GPIO_MODE_ANALOG; // Аналоговый режим
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_PORT, &g);
}

// Инициализация UART 
static void UART2_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE(); // Включаем тактирование USART2

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX; // Только передача
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

// Инициализация АЦП
static void ADC_Init(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4; // Делим тактовую частоту для АЦП
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;           // 12-битное разрешение (0-4095)
    hadc1.Init.ScanConvMode = DISABLE;                    // Одно преобразование за раз
    hadc1.Init.ContinuousConvMode = DISABLE;              // Не непрерывный режим
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING; // Запуск по фронту
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;        // Триггер от таймера TIM2
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);

    // Настройка регулярного канала (потенциометр на PA0)
    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel = ADC_CHANNEL_0;
    ch.Rank = 1;
    ch.SamplingTime = ADC_SAMPLETIME_480CYCLES; // Долгое время выборки для стабильности
    HAL_ADC_ConfigChannel(&hadc1, &ch);

    // Настройка инжектированной группы (Внутренний источник опорного напряжения VREFINT)
    // Для измерения реального напряжения питания VDDA
    ADC_InjectionConfTypeDef inj = {0};
    inj.InjectedChannel = ADC_CHANNEL_VREFINT;
    inj.InjectedRank = ADC_INJECTED_RANK_1;
    inj.InjectedNbrOfConversion = 1;
    inj.InjectedSamplingTime = ADC_SAMPLETIME_480CYCLES;
    inj.InjectedOffset = 0;
    inj.InjectedDiscontinuousConvMode = DISABLE;
    inj.AutoInjectedConv = DISABLE;
    inj.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONVEDGE_RISING;
    inj.ExternalTrigInjecConv = ADC_EXTERNALTRIGINJECCONV_T5_TRGO; // Триггер от таймера TIM5
    HAL_ADCEx_InjectedConfigChannel(&hadc1, &inj);

    // Настройка прерываний АЦП
    HAL_NVIC_SetPriority(ADC_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(ADC_IRQn);
}

// Инициализация таймера для генерации триггеров (TRGO) для АЦП
static void TimerTrgo_Init(TIM_HandleTypeDef *h, TIM_TypeDef *inst, uint32_t period_ms)
{
    h->Instance = inst;
    h->Init.Prescaler = 16000 - 1; // 16 МГц / 16000 = 1 кГц (1 тик = 1 мс)
    h->Init.CounterMode = TIM_COUNTERMODE_UP;
    h->Init.Period = period_ms - 1; // Задаем период в миллисекундах
    h->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    h->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(h);

    __HAL_TIM_CLEAR_FLAG(h, TIM_FLAG_UPDATE);

    // Настраиваем таймер на генерацию события обновления (Update Event) как триггер (TRGO)
    TIM_MasterConfigTypeDef m = {0};
    m.MasterOutputTrigger = TIM_TRGO_UPDATE;
    m.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(h, &m);
    __HAL_TIM_CLEAR_FLAG(h, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start(h);
}

void Error_Handler(void);

int main(void)
{
    HAL_Init();
    GPIO_Init();
    UART2_Init();

    // Включаем тактирование таймеров и настраиваем их
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM5_CLK_ENABLE();
    TimerTrgo_Init(&htim2, TIM2, ADC_PERIOD_MS);  // TIM2 запускает опрос потенциометра
    TimerTrgo_Init(&htim5, TIM5, VREF_PERIOD_MS); // TIM5 запускает замер VREFINT
    ADC_Init();

    // Отключаем буферизацию вывода (текст пойдет сразу)
    setvbuf(stdout, NULL, _IONBF, 0);

    // Запускаем АЦП в режиме прерываний
    HAL_ADC_Start_IT(&hadc1);           
    HAL_ADCEx_InjectedStart_IT(&hadc1); 

    // Вывод заголовка таблицы в терминал
    printf("RAW  U_manual(mV)  U_cali(mV)  Error(%%)\r\n");
    printf("-----------------------------------------\r\n");

    uint32_t last_print_time = 0; // Переменная для таймера вывода (раз в 2 сек)

    while (1)
    {
        // Если в прерывании пришли новые данные с АЦП
        if (g_new)
        {
            // Копируем значения в локальные переменные, чтобы избежать конфликтов с прерываниями
            uint16_t raw = g_raw;
            uint32_t vdda = g_vdda_mv;
            g_new = false;

            // Проверяем, прошло ли 2 секунды с момента последнего вывода
            if (HAL_GetTick() - last_print_time >= PRINT_PERIOD_MS)
            {
                last_print_time = HAL_GetTick(); // Запоминаем время вывода

                // Вычисляем напряжение по номинальной формуле (опорное 3.3В)
                float u_manual = (float)raw * VDDA_NOMINAL_MV / 4095.0f;
                
                // Вычисляем напряжение с учетом реального измеренного VDDA
                float u_cali   = (float)raw * vdda / 4095.0f;
                
                // Вычисляем относительную погрешность в процентах: 
                float error    = fabsf(u_manual - u_cali) / u_cali * 100.0f;

                // Умножаем на 10 и 100, чтобы получить целые числа для вывода
                int um_x10   = (int)(u_manual * 10.0f); // Например: 412.1 -> 4121
                int uc_int   = (int)(u_cali);           // Отбрасываем дробную часть для U_cali
                int err_x100 = (int)(error * 100.0f);   // Например: 3.55 -> 355

                // Выводим данные, вставляя точку вручную (деление и остаток от деления)
                printf("%-5u %d.%d\t%-11d %d.%02d\r\n", 
                       (unsigned)raw,                  // RAW значение
                       um_x10 / 10, um_x10 % 10,       // U_manual: целая часть и десятая доля
                       uc_int,                         // U_cali: только целая часть
                       err_x100 / 100, err_x100 % 100  // Error: целая часть и сотые доли (%02d для ведущего нуля)
                );
            }
        }
    }
}


/* ---------------- Обработчики прерываний ---------------- */

// Системный таймер 
void SysTick_Handler(void)
{
    HAL_IncTick();
}

// Перенаправление стандартного вывода (printf) в UART
int _write(int fd, char *ptr, int len)
{
    (void)fd;
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);
    return len;
}

// Общий обработчик прерываний АЦП
void ADC_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&hadc1);
}

// Колбэки 

// Колбэк окончания преобразования регулярной группы (срабатывает каждые 100 мс)
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (ADC1 == hadc->Instance)
    {
        // Читаем сырое значение
        g_raw = (uint16_t)HAL_ADC_GetValue(hadc);
        g_new = true; // Сообщаем главному циклу, что есть новые данные

        // --- Логика управления светодиодом с гистерезисом ---
        // Если было светло, и стало темнее порога TH_DARK -> включаем светодиод
        if (!g_dark && g_raw < TH_DARK)
        {
            g_dark = true;
            led_set(1);
        }
        // Если было темно, и стало светлее порога TH_LIGHT -> выключаем светодиод
        else if (g_dark && g_raw > TH_LIGHT)
        {
            g_dark = false;
            led_set(0);
        }
    }
}

// Колбэк окончания инжектированного преобразования (замер VREFINT каждые 100 мс)
void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    // Статические переменные для скользящего среднего
    static uint16_t buf[VREFINT_AVG_N];
    static uint32_t sum = 0;
    static uint8_t idx = 0;
    static uint8_t cnt = 0;

    if (ADC1 != hadc->Instance)
    {
        return;
    }

    // Получаем значение внутреннего опорного напряжения
    uint16_t data = (uint16_t)HAL_ADCEx_InjectedGetValue(hadc, ADC_INJECTED_RANK_1);
    
    // Обновляем скользящее среднее по кольцевому буферу
    sum -= buf[idx];
    buf[idx] = data;
    sum += data;
    idx = (idx + 1) % VREFINT_AVG_N;

    // Считаем количество накопленных значений (максимум VREFINT_AVG_N)
    if (cnt < VREFINT_AVG_N)
    {
        cnt++;
    }

    // Вычисляем среднее значение
    uint32_t avg = sum / cnt;

    // Вычисляем реальное напряжение питания VDDA по формуле из даташита
    if (0 != avg)
    {
        g_vdda_mv = VDDA_NOMINAL_MV * (*VREFINT_CAL_ADDR) / avg;
    }
}

// Заглушка для обработки критических ошибок
void Error_Handler(void)
{
    __disable_irq(); // Отключаем все прерывания
    while (1)        // Вечный цикл (зависание)
    {
    }
}