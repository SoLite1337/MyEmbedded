#define SYSTEM_STATE_H

 // НАЗНАЧЕНИЕ:
 //   Зачем нужен:
 //    - отладка;
 //    - шина расширения
 //
 //   Это дополнительный слой НАД суб-FSM (button/led/thermal/combat/alarm), 
 // Сами подсистемы остаются независимыми.

typedef enum 
{
    SYS_ST_BOOT = 0,        // инициализация
    SYS_ST_READY,           // все подсистемы в IDLE - система готова
    SYS_ST_THERMAL_ACTIVE,  // идет серия тепловых ловушек
    SYS_ST_COMBAT_LOCKED,   // цель захвачена, ждем пуска
    SYS_ST_COMBAT_LAUNCH,   // идет пуск ракеты (мигание зелёного)
    SYS_ST_SENSOR_WARNING,  // предупреждение датчика облучения
    SYS_ST_SENSOR_ALARM,    // тревога — система «обнаружена»
    SYS_ST_COMBINED,        // одновременно активны ≥2 подсистем
} system_state_t;

void system_state_init(void);                   // инциализация (SYS_ST_BOOT)
void system_state_tick(void);                   // вычисление нового состояния (вызывать раз в тик)
system_state_t system_state_get(void);          // получить текущее состояние
//const char *system_state_name(system_state_t s);   //логирование (не реализовано)