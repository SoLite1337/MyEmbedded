#include "system_state.h"
#include "thermal_traps.h"
#include "combat_system.h"
#include "alarm_system.h"

// НАЗНАЧЕНИЕ:
// Агрегирующий FSM, который определяет общее состояние системы на основе состояний подсистем.
// Приоритеты:
//
//     - если активны ≥ 2 подсистемы, то SYS_ST_COMBINED;
//     - иначе берём состояние единственной активной подсистемы;
//     - если ни одна не активна, то SYS_ST_READY.
//


static system_state_t s_state;

// Инициализация состояния системы
void system_state_init(void)
{
    s_state = SYS_ST_BOOT;
}

// Вычисление нового состояния системы на основе состояний подсистем
void system_state_tick(void)
{
    int active = 0;
    system_state_t single = SYS_ST_READY;

    if (thermal_traps_is_active()) 
    {
        active++;
        single = SYS_ST_THERMAL_ACTIVE;
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
    } else 
    {
        s_state = SYS_ST_COMBINED;
    }
}

system_state_t system_state_get(void) // получить текущее состояние
{
    return s_state;
}

// const char *system_state_name(system_state_t s)
// {
//     switch (s) {
//     case SYS_ST_BOOT:           return "BOOT";
//     case SYS_ST_READY:          return "READY";
//     case SYS_ST_THERMAL_ACTIVE: return "THERMAL_ACTIVE";
//     case SYS_ST_COMBAT_LOCKED:  return "COMBAT_LOCKED";
//     case SYS_ST_COMBAT_LAUNCH:  return "COMBAT_LAUNCH";
//     case SYS_ST_SENSOR_WARNING: return "SENSOR_WARNING";
//     case SYS_ST_SENSOR_ALARM:   return "SENSOR_ALARM";
//     case SYS_ST_COMBINED:       return "COMBINED";
//     default:                    return "?";
//     }
// }