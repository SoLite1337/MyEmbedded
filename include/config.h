#pragma once

//Пины
#define LED_RED     18
#define LED_GREEN   21
#define LDR         2
#define BTN_ARM     17
#define BTN_RESET   16

//Периоды опроса
#define LDR_PERIOD_MS   1000
#define BTN_PERIOD_MS   20

//Пороги
#define LDR_THRESHOLD    3750   // всё, что выше — считаем лазером
#define BLINK_PERIOD_MS  100    // период мигания красного LED

//Фильтр
#define LDR_FILTER_N  10        // было 32, уменьшил для быстрой реакции