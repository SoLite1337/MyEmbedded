#ifndef COMBAT_SYSTEM_H
#define COMBAT_SYSTEM_H
#include <stdint.h>

#define CS_BLINK_MS     250u
#define CS_BLINK_COUNT  3u

typedef enum 
{
    CS_ST_IDLE, 
    CS_ST_LOCKED, 
    CS_ST_LAUNCHING,
} cs_state_t;

// Структура для управления боевой системой
void combat_system_init(void);
void combat_system_tick(void);
uint8_t combat_system_is_active(void);
cs_state_t combat_system_get_state(void);

#endif