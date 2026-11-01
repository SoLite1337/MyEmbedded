#include "stm32f4xx_hal.h"
#include <stdbool.h>

//================================================================
// Режим работы:
// 1 - наладка/тестовый режим (ускоренные интервалы)
// 0 - рабочий режим
#define TEST_MODE 1
#if TEST_MODE
    // Тестовый режим: вентилятор 3 секунды работает, 9 секунд стоит
    #define FAN_ON_TIME_MS       3000U
    #define FAN_OFF_TIME_MS      9000U
#else
    // Рабочий цикл: 60 мин total.
    // 15 мин вентилятор работает + 45 мин стоит
    #define FAN_ON_TIME_MS       900000U
    #define FAN_OFF_TIME_MS      2700000U
#endif

// Настройка пинов
// PB1 — управление базой транзистора на двигатель вентилятора
#define FAN_PORT GPIOB
#define FAN_PIN  GPIO_PIN_1

// PC13 — индикация через встроенный светодиод (активный низкий уровень)
#define LED_PORT GPIOC
#define LED_PIN  GPIO_PIN_13
//================================================================

// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// Глобальный дескриптор таймера TIM2.
// Нужен, чтобы к нему был доступ из main(), IRQHandler и callback.
TIM_HandleTypeDef htim2;

// Текущее состояние вентилятора.
volatile bool is_fan_on = false;


// ПРОТОТИПЫ ФУНКЦИЙ
void SystemClock_Config(void);      // Настройка системного тактирования
static void GPIO_Init(void);        // Настройка GPIO для вентилятора и LED
static void TIM2_Init(void);        // Настройка TIM2 для формирования интервалов
void Error_Handler(void);           // Аварийный останов при ошибке



// ГЛАВНАЯ ФУНКЦИЯ
// Инициализация, задаёт начальное состояние вентилятора и LED, запускает TIM2 с прерываниями.
// Дальше вся логика переключения выполняется в callback таймера.

int main(void)
{
    // Инициализация HAL
    HAL_Init();
    // Настройка системной частоты
    SystemClock_Config();
    // Инициализация GPIO
    GPIO_Init();
    // Инициализация TIM2
    TIM2_Init();



    // Начальное состояние: вентилятор выключен
    is_fan_on = false;

    // Явно выставляем низкий уровень на PB1 - вентилятор выключен
    HAL_GPIO_WritePin(FAN_PORT, FAN_PIN, GPIO_PIN_RESET);

    // Выключаем встроенный LED. PC13 активен низким уровнем, поэтому SET = выключено.
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    
    // Первый интервал время простоя. Записываем период в ARR таймера.
    __HAL_TIM_SET_AUTORELOAD(&htim2, FAN_OFF_TIME_MS - 1U);

    // Сбрасываем счётчик таймера, чтобы интервал начался с нуля
    __HAL_TIM_SET_COUNTER(&htim2, 0U);

    // Запуск TIM2 с прерываниями по переполнению
    if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    while (1)
    {

    }
}




// ИНИЦИАЛИЗАЦИЯ GPIO
// Тактирование портов GPIOB/GPIOС:
// PB1 как выход для управления вентилятором;
// PC13 как выход для встроенного светодиода.

static void GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    // Включение тактирования портов PB и PC
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();


    // ------------------------------------------------------------------------
    // PB1 — управление вентилятором
    // ------------------------------------------------------------------------
    // Перед настройкой выхода заранее выставляем низкий уровень, чтобы вентилятор не дёрнулся при инициализации.
    HAL_GPIO_WritePin(FAN_PORT, FAN_PIN, GPIO_PIN_RESET);
    gpio.Pin = FAN_PIN;                         // Пин PB1
    gpio.Mode = GPIO_MODE_OUTPUT_PP;            // Выход push-pull
    gpio.Pull = GPIO_NOPULL;                    // Без подтяжки
    gpio.Speed = GPIO_SPEED_FREQ_LOW;           // Низкая скорость переключения
    HAL_GPIO_Init(FAN_PORT, &gpio);


    // ------------------------------------------------------------------------
    // PC13 — встроенный светодиод
    // ------------------------------------------------------------------------
    // PC13 активен низким уровнем. SET - светодиод выключен.
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    gpio.Pin = LED_PIN;                         // Пин PC13
    gpio.Mode = GPIO_MODE_OUTPUT_PP;            // Выход push-pull
    gpio.Pull = GPIO_NOPULL;                    // Без подтяжки
    gpio.Speed = GPIO_SPEED_FREQ_LOW;           // Низкая скорость переключения

    HAL_GPIO_Init(LED_PORT, &gpio);
}





// ИНИЦИАЛИЗАЦИЯ TIM2
// Настраивает TIM2 так, чтобы один тик соответствовал 1 мс, 
// + задаёт начальный период, настраивает приоритет и разрешает прерывание TIM2.

static void TIM2_Init(void)
{
    // Включение тактирования TIM2
    __HAL_RCC_TIM2_CLK_ENABLE();
    htim2.Instance = TIM2;

    // HSI = 16 МГц.
    // Делитель 16000 формирует частоту счетчика 1 кГц.
    // Один полный отсчет таймера соответствует 1 мс.
    htim2.Init.Prescaler = 16000U - 1U;

    // Счётчик считает вверх
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;

    // Начальный период время простоя вентилятора
    htim2.Init.Period = FAN_OFF_TIME_MS - 1U;

    // Делитель тактовой частоты для цифрового фильтра - без деления
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

    // Предзагрузка ARR отключена: новый период применяется сразу после записи в ARR.
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    // Инициализация таймера
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }


    // Настройка приоритета прерывания TIM2
    HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);

    // Разрешаем прерывание TIM2 в NVIC
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}


// ОБРАБОТЧИК ПРЕРЫВАНИЯ TIM2
// Передаёт прерывание в стандартный обработчик HAL.
// Основная логика находится в HAL_TIM_PeriodElapsedCallback().
 
void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim2);
}






// CALLBACK: ЗАВЕРШЕНИЕ ПЕРИОДА TIM2
// Вызывается HAL при переполнении TIM2.
// Переключает состояние вентилятора и LED, а также задаёт следующий интервал:
//  - если вентилятор включён - следующий период равен времени работы;
//  - если выключен - следующий период равен времени простоя.

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    // Проверяем, что прерывание пришло именно от TIM2
    if (htim->Instance == TIM2)
    {
        // Переключаем состояние вентилятора
        is_fan_on = !is_fan_on;
        if (is_fan_on)
        {
            // ВЕНТИЛЯТОР ВКЛЮЧЕН
            // Включаем вентилятор: высокий уровень на PB1
            HAL_GPIO_WritePin(FAN_PORT, FAN_PIN, GPIO_PIN_SET);

            // LED активен низким уровнем
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

            // Следующий интервал время работы вентилятора
            __HAL_TIM_SET_AUTORELOAD(htim, FAN_ON_TIME_MS - 1U);
        }
        else
        {
            // ВЕНТИЛЯТОР ВЫКЛЮЧЕН
            // Выключаем вентилятор: низкий уровень на PB1
            HAL_GPIO_WritePin(FAN_PORT, FAN_PIN, GPIO_PIN_RESET);

            // Выключаем LED
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);

            // Следующий интервал время простоя
            __HAL_TIM_SET_AUTORELOAD(htim, FAN_OFF_TIME_MS - 1U);
        }


        // Сбрасываем счётчик для нового интервала
        __HAL_TIM_SET_COUNTER(htim, 0U);
    }
}




// НАСТРОЙКА СИСТЕМНОГО ТАКТИРОВАНИЯ
// включает внутренний генератор HSI 16 МГц, настраивает
// источник SYSCLK и делители шин AHB/APB1/APB2. PLL не используется.

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};


    // Включение тактирования блока управления питанием PWR
    __HAL_RCC_PWR_CLK_ENABLE();

    // Устанавливаем масштаб напряжения регулятора
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);


    // Используем внутренний генератор HSI 16 МГц
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;

    RCC_OscInitStruct.HSIState = RCC_HSI_ON;

    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;

    // PLL не используется
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;


    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }


    // Настройка шин тактирования
    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK   |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1  |
        RCC_CLOCKTYPE_PCLK2;

    // Источник SYSCLK — HSI
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;

    // Делители шин: без деления
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;


    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}


// ОБРАБОТКА ОШИБОК
// При критической ошибке запрещает прерывания и уходит
// в бесконечный цикл. Такой режим удобен для отладки: программа
// останавливается в известной точке.

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        // Останов если ошибка
    }
}