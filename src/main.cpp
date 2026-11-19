#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdio.h>

#define LED_PORT GPIOA
#define LED_PIN GPIO_PIN_1 /* TIM2_CH2, AF1 */

#define PWM_FREQ_HZ 1000U
#define TIM_TICK_HZ 1000000U  /* 16 MHz / PSC 16 */
#define TIM_FREQ_HZ 16000000U /* 16 MHz */
#define PWM_PSC 16U
#define PWM_ARR (TIM_TICK_HZ / PWM_FREQ_HZ - 1U)

/* Строгий 9-нотный звукоряд Большой шотландской волынки (A Mixolydian) */
#define BAG_LOW_G  392  /* G4 (нижняя соль) */
#define BAG_LOW_A  440  /* A4 (тоника / бурдон) */
#define BAG_B      494  /* B4 */
#define BAG_C      554  /* C#5 (в нотах волынки пишется как C, звучит C#) */
#define BAG_D      587  /* D5 */
#define BAG_E      659  /* E5 */
#define BAG_F      740  /* F#5 (в нотах волынки пишется как F, звучит F#) */
#define BAG_HIGH_G 784  /* G5 (верхняя соль) */
#define BAG_HIGH_A 880  /* A5 (верхняя ля) */

/* Длительности для марша горцев (~100 BPM) */
#define SNAP_SHORT 130  /* Короткая нота в "шотландском снэпе" */
#define SNAP_LONG  430  /* Длинная нота после снэпа */
#define EIGHTH     280  /* Обычная восьмая */
#define QTR        580  /* Четверть */
#define DOT_Q      880  /* Четверть с точкой */
#define HALF       1180 /* Половинная */

/* ---------- globals (shared with ISR context) ---------- */
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart2;

/* ---------- helpers ---------- */

static void GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};

    g.Pin = LED_PIN;
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    g.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(LED_PORT, &g);

    g.Pin = GPIO_PIN_2; /* USART2 TX */
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &g);
}

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

static void TIM2_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = PWM_PSC - 1; /* 16 MHz / 16 = 1 MHz */
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = PWM_ARR; /* 1 MHz / 1000 = 1 kHz */
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim2);

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 0;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &oc, TIM_CHANNEL_2);

    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

static void set_tone_freq(int freq_hz)
{
    if (0 == freq_hz)
    {
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
        return;
    }

    uint32_t f_clk = TIM_FREQ_HZ;
    uint32_t psc = PWM_PSC - 1;
    uint32_t arr = (f_clk / (psc + 1)) / freq_hz - 1;

    __HAL_TIM_SET_AUTORELOAD(&htim2, arr);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, arr / 2);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

/* Основная нота волынки: почти без паузы, т.к. у волынки непрерывный поток воздуха */
static void pipe_note(int freq_hz, int duration_ms)
{
    set_tone_freq(freq_hz);
    HAL_Delay(duration_ms);
    set_tone_freq(0);
    HAL_Delay(12);
}

/* Фирменный волыночный форшлаг (G-grace note): мгновенный щелчок верхней нотой без паузы */
static void pipe_grace(int grace_hz, int main_hz, int duration_ms)
{
    set_tone_freq(grace_hz);
    HAL_Delay(28);
    pipe_note(main_hz, duration_ms);
}

/* Часть A: Клановый походный марш (с "шотландским снэпом" и бурдонным возвратом на Low G - Low A) */
static void play_highland_march_a(void)
{
    /* Затакт */
    pipe_note(BAG_LOW_A, EIGHTH);

    /* Такт 1: Снэп A4 -> C#5, затем E5 с форшлагом G5 */
    pipe_note(BAG_LOW_A, SNAP_SHORT);
    pipe_note(BAG_C, SNAP_LONG);
    pipe_grace(BAG_HIGH_G, BAG_E, QTR);
    pipe_note(BAG_C, EIGHTH);
    pipe_note(BAG_E, EIGHTH);

    /* Такт 2: F#5 (с форшлагом A5) - E5 - C#5 - B4 */
    pipe_grace(BAG_HIGH_A, BAG_F, DOT_Q);
    pipe_note(BAG_E, EIGHTH);
    pipe_note(BAG_C, QTR);
    pipe_note(BAG_B, QTR);

    /* Такт 3: Снэп Low G -> B4 (характерный шотландский ход!) -> D5 -> B4 */
    pipe_note(BAG_LOW_G, SNAP_SHORT);
    pipe_note(BAG_B, SNAP_LONG);
    pipe_grace(BAG_HIGH_G, BAG_D, QTR);
    pipe_note(BAG_B, EIGHTH);
    pipe_note(BAG_D, EIGHTH);

    /* Такт 4: E5 - C#5 - B4 - Low A */
    pipe_note(BAG_E, SNAP_SHORT);
    pipe_note(BAG_C, SNAP_LONG);
    pipe_note(BAG_B, QTR);
    pipe_note(BAG_LOW_A, QTR);

    /* Такт 5: Повтор с усилением: Снэп A4 -> C#5 -> E5 -> F#5 */
    pipe_note(BAG_LOW_A, SNAP_SHORT);
    pipe_note(BAG_C, SNAP_LONG);
    pipe_grace(BAG_HIGH_G, BAG_E, QTR);
    pipe_note(BAG_F, QTR);

    /* Такт 6: High A5 (пик фразы!) - F#5 - E5 - C#5 */
    pipe_grace(BAG_HIGH_G, BAG_HIGH_A, DOT_Q);
    pipe_note(BAG_F, EIGHTH);
    pipe_note(BAG_E, QTR);
    pipe_note(BAG_C, QTR);

    /* Такт 7: Снэп B4 -> D5 -> C#5 -> Low G */
    pipe_note(BAG_B, SNAP_SHORT);
    pipe_note(BAG_D, SNAP_LONG);
    pipe_note(BAG_C, QTR);
    pipe_note(BAG_LOW_G, QTR);

    /* Такт 8: Разрешение в тонику Low A с двойным ударом (doubling) */
    pipe_grace(BAG_HIGH_G, BAG_LOW_A, HALF);
    pipe_grace(BAG_D, BAG_LOW_A, QTR);
    pipe_note(BAG_E, EIGHTH);
    pipe_note(BAG_F, EIGHTH);
}

/* Часть B: Боевой клич горцев (верхний регистр волынки: E5 - F#5 - G5 - A5) */
static void play_highland_march_b(void)
{
    /* Такт 9: High A5 (тянется) - F#5 - High A5 */
    pipe_grace(BAG_HIGH_G, BAG_HIGH_A, HALF);
    pipe_note(BAG_F, QTR);
    pipe_note(BAG_HIGH_A, QTR);

    /* Такт 10: High G5 (миксолидийская 7-я ступень!) - E5 - High G5 - E5 */
    pipe_note(BAG_HIGH_G, DOT_Q);
    pipe_note(BAG_E, EIGHTH);
    pipe_grace(BAG_HIGH_A, BAG_HIGH_G, QTR);
    pipe_note(BAG_E, QTR);

    /* Такт 11: F#5 - High A5 - F#5 - E5 */
    pipe_note(BAG_F, SNAP_SHORT);
    pipe_note(BAG_HIGH_A, SNAP_LONG);
    pipe_note(BAG_F, QTR);
    pipe_note(BAG_E, QTR);

    /* Такт 12: C#5 - E5 - B4 - Low G */
    pipe_note(BAG_C, SNAP_SHORT);
    pipe_note(BAG_E, SNAP_LONG);
    pipe_note(BAG_B, QTR);
    pipe_note(BAG_LOW_G, QTR);

    /* Такт 13: High A5 - F#5 - E5 - C#5 */
    pipe_grace(BAG_HIGH_G, BAG_HIGH_A, DOT_Q);
    pipe_note(BAG_F, EIGHTH);
    pipe_note(BAG_E, QTR);
    pipe_note(BAG_C, QTR);

    /* Такт 14: D5 - F#5 - High G5 - B4 */
    pipe_note(BAG_D, SNAP_SHORT);
    pipe_note(BAG_F, SNAP_LONG);
    pipe_note(BAG_HIGH_G, QTR);
    pipe_note(BAG_B, QTR);

    /* Такт 15: Снэп C#5 -> E5 -> Low G -> B4 */
    pipe_note(BAG_C, SNAP_SHORT);
    pipe_note(BAG_E, SNAP_LONG);
    pipe_note(BAG_LOW_G, QTR);
    pipe_note(BAG_B, QTR);

    /* Такт 16: Финальный протяжный бурдонный аккорд-унисон Low A */
    pipe_grace(BAG_HIGH_G, BAG_LOW_A, HALF + QTR);
    pipe_grace(BAG_D, BAG_LOW_A, QTR);
}

void Error_Handler(void);

int main(void)
{
    HAL_Init();
    GPIO_Init();
    UART2_Init();
    TIM2_Init();

    setvbuf(stdout, NULL, _IONBF, 0);

    while (1)
    {
        /* Традиционный шотландский марш: A - A - B - B */
        play_highland_march_a();
        play_highland_march_a();
        play_highland_march_b();
        play_highland_march_b();

        HAL_Delay(2000);
    }
}

#ifdef __cplusplus
extern "C"
{
#endif
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