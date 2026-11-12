#include "bl/system_state.h"
#include "cl/thermal_traps.h"
#include "cl/combat_system.h"
#include "cl/alarm_system.h"

static system_state_t s_state;

void system_state_init(void) 
{ 
    s_state = SYS_ST_BOOT; 
}

void system_state_tick(void) 
{
    int active = 0;
    system_state_t single = SYS_ST_READY;

    if (thermal_traps_is_active()) 
    {
        active++; single = SYS_ST_THERMAL_ACTIVE;
    }
    if (combat_system_is_active()) 
    {
        active++;
        single = (combat_system_get_state() == CS_ST_LAUNCHING) ? SYS_ST_COMBAT_LAUNCH : SYS_ST_COMBAT_LOCKED;
    }
    if (alarm_system_is_active()) 
    {
        active++;
        single = (alarm_system_get_state() == AS_ST_ALARM) ? SYS_ST_SENSOR_ALARM : SYS_ST_SENSOR_WARNING;
    }

    if (active == 0) 
    {
        s_state = SYS_ST_READY;
    }
    else if (active == 1) 
    {
        s_state = single;
    }
    else 
    {
        s_state = SYS_ST_COMBINED;
    }
}

system_state_t system_state_get(void) 
{ 
    return s_state; 
}