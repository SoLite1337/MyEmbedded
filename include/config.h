#pragma once

// ---- Пины ----
#define LDR_PIN     2      // ADC1_CH1 (GPIO2 на ESP32-S3)
#define RELAY_PIN   15     // выход на транзистор VT1 (BC547B)

// ---- Гистерезис (значения ADC 0..4095) ----
#define THRESHOLD_DARK    1500   // ниже — считаем «темно»
#define THRESHOLD_LIGHT   2500   // выше — считаем «светло»
                                 // между — держим текущее состояние

// ---- Периоды ----
#define LDR_PERIOD_MS       50   // опрос ADC
#define LDR_LOG_PERIOD_MS  500   // вывод в Serial (чтобы не спамить)

// ---- Фильтр ----
#define LDR_FILTER_N  10

// ---- Полярность реле ----
// 1 = активный HIGH (GPIO HIGH -> реле ON)
// 0 = активный LOW  (GPIO LOW  -> реле ON)
// Многие модули реле — активные LOW, тогда поставь 0.
#define RELAY_ACTIVE_HIGH 0