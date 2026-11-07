#define COMBAT_SYSTEM_H
#include <stdint.h>

// НАЗНАЧЕНИЕ:
//   Боевой режим (зеленый LED).
//     - LONG BTN_COMBAT    - захват цели (LED госветитрит постоянно);
//     - SHORT BTN_COMBAT   - пуск ракеты (4 мигания 250/250 мс, затем LED гаснет, возврат в IDLE).

#define CS_BLINK_MS     250u
#define CS_BLINK_COUNT  4u

// Состояния FSM боевого режима.
typedef enum 
{
    CS_ST_IDLE,        // Ожидвание LP
    CS_ST_LOCKED,      // цель захвачена (зелёный светит)       
    CS_ST_LAUNCHING,   // идёт пуск - серия миганий зелёного    
} cs_state_t;

void combat_system_init(void);               // иницлиализация, перевод в IDLE
void combat_system_tick(void);               // Один шаг FSM. Вызывать раз в 1 мс из app_tick() (TIM3 ISR).
uint8_t combat_system_is_active(void);       // Возвращает 1, если FSM не в IDLE, иначе 0.
cs_state_t combat_system_get_state(void);    // Возвращает текущее состояние FSM боевого режима.