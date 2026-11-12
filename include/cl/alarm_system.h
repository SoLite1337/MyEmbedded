#define ALARM_SYSTEM_H
#include <stdint.h>

#define AS_BLINK_MS  250u

// Состояния системы сигнализации
typedef enum 
{
    AS_ST_IDLE, 
    AS_ST_WARNING, 
    AS_ST_ALARM,
} as_state_t;


// Структура для управления системой сигнализации
void alarm_system_init(void);
void alarm_system_tick(void);
uint8_t alarm_system_is_active(void);
as_state_t alarm_system_get_state(void);