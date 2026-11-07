#define BUTTON_H
#include <stdint.h>
#include "stm32f4xx_hal.h"

 // НАЗНАЧЕНИЕ:
 //   Драйвер кнопки
 //   Реализован как FSM. 
 //   Вход в машину через EXTI-ISR (button_irq), 
 //   шаг машины выполняется из app_tick() каждую 1 мс (button_tick). 
 //   Наружу модуль отдаёт события через button_get_event.

#define BTN_DEBOUNCE_MS    20u   // - время стабилизации контакта
#define BTN_LONG_PRESS_MS  2000u // - порог длинного нажатия




// События для FSM кнопки
typedef enum 
{
    BTN_EVT_NONE = 0,       // без события
    BTN_EVT_SHORT_PRESS,    // короткое нажатие
    BTN_EVT_LONG_PRESS,     // удержание дольше BTN_LONG_PRESS_MS
} btn_event_t;

// Состояния FSM кнопки
typedef enum 
{
    BTN_ST_IDLE,             // кнопка не нажата, ждем фронт
    BTN_ST_DEBOUNCE_PRESS,   // нажатие, ждем стабилизации
    BTN_ST_HELD,             // нажата, идет отсчёт до «длинного»
    BTN_ST_HELD_LONG,        // нажата, длинное уже сгенерировано
    BTN_ST_DEBOUNCE_RELEASE, // отпускание, ждём стабилизации
} btn_state_t;

// Контекст одной кнопки. Экземпляры создаются в app.c.
typedef struct 
{
    GPIO_TypeDef     *port;          // порт кнопки
    uint16_t          pin;           // номер пина
    btn_state_t       state;         // текущее состояние FSM
    uint32_t          timer;         // таймер текущей фазы (мс)
    uint32_t          saved_timer;   // откат при ложном релизе
    uint8_t           long_detected; // фиксируем LONG PRESS
    volatile uint8_t  edge_irq;      // флаг, ставится в EXTI ISR
    btn_event_t       pending;       // ожидающее событие для наружу
} button_t;

// функции для работы с кнопкой
void button_init(button_t *b, GPIO_TypeDef *port, uint16_t pin); // Привязать объект-кнопку к конкретному пину и сбросить состояние.
void button_irq(button_t *b);                                    // Сообщить FSM, что на пине был фронт
void button_tick(button_t *b);                                   // Продвинуть FSM на один шаг (1 мс).
btn_event_t button_get_event(button_t *b);                       // Забрать накопившееся событие (SHORT / LONG / NONE) 
btn_state_t button_get_state(const button_t *b);                 // Прочитать текущее состояние FSM 
