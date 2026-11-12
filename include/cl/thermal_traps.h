#define THERMAL_TRAPS_H
#include <stdint.h>

// Параметры тепловых ловушек
#define TT_ON_MS     100u
#define TT_OFF_MS    250u
#define TT_COUNT     3u // Виправив на 3, як було в ТЗ

// Состояния тепловых ловушек
typedef enum 
{
    TT_ST_IDLE, 
    TT_ST_BURST_ON, 
    TT_ST_BURST_OFF,
} tt_state_t;

void thermal_traps_init(void);
void thermal_traps_tick(void);
uint8_t thermal_traps_is_active(void);
tt_state_t thermal_traps_get_state(void);