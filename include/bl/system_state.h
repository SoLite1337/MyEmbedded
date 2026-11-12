#define SYSTEM_STATE_H

typedef enum 
{
    SYS_ST_BOOT = 0,
    SYS_ST_READY,
    SYS_ST_THERMAL_ACTIVE,
    SYS_ST_COMBAT_LOCKED,
    SYS_ST_COMBAT_LAUNCH,
    SYS_ST_SENSOR_WARNING,
    SYS_ST_SENSOR_ALARM,
    SYS_ST_COMBINED,
} system_state_t;

void system_state_init(void);
void system_state_tick(void);
system_state_t system_state_get(void);
