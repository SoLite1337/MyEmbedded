#define BUTTON_H
#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "core/event_bus.h" 

#define BTN_DEBOUNCE_MS    20u
#define BTN_LONG_PRESS_MS  2000u

// Состояния кнопки
typedef enum 
{
    BTN_ST_IDLE,
    BTN_ST_DEBOUNCE_PRESS,
    BTN_ST_HELD,
    BTN_ST_HELD_LONG,
    BTN_ST_DEBOUNCE_RELEASE,
} btn_state_t;


// Структура для управления кнопкой
typedef struct 
{
    GPIO_TypeDef     *port;
    uint16_t          pin;
    btn_state_t       state;
    uint32_t          timer;
    uint32_t          saved_timer;
    uint8_t           long_detected;
    volatile uint8_t  edge_irq;
    
    // События, которые будут опубликованы при коротком и длинном нажатии
    app_event_t       evt_short;
    app_event_t       evt_long;
} button_t;

// Инициализация кнопки
void button_init(button_t *b, GPIO_TypeDef *port, uint16_t pin, app_event_t evt_s, app_event_t evt_l);
void button_irq(button_t *b);
void button_tick(button_t *b);