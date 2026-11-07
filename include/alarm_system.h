#define ALARM_SYSTEM_H
#include <stdint.h>

//   НАЗНАЧЕНИЕ:
//   Подсистема датчика облучения и тревоги (желтый LED).
//     - короткий BTN_SENSOR из IDLE   = WARNING (желтый светит всегда);
//     - длинный BTN_SENSOR из IDLE    = ALARM (желтый мигает 250/250);
//     - короткое нажатие в WARNING    = IDLE (желтый гаснет);
//     - длинное нажатие  в WARNING    = ALARM;
//
//     - из ALARM выйти можно ТОЛЬКО через скид тепловых ловушек:
//      т.е. когда thermal_traps сообщает выставляет burst_done - переход в IDLE.
// ==================================================================

#define AS_BLINK_MS  250u

// Состояния FSM тревоги.
typedef enum 
{
    AS_ST_IDLE,      // ожидание импульса от BTN_SENSOR
    AS_ST_WARNING,   // Предупреждение (наприпер. попытка по мне прицелицелиться или ложное срабатывание) - LED горит
    AS_ST_ALARM,     // состояние «вас зафиксировали» - LED мигает и ожидает квитирование           
} as_state_t;

void        alarm_system_init(void); // инициализация подсистемы тревоги
void        alarm_system_tick(void);  // обработка событий подсистемы тревоги (вызывается из main_loop)
uint8_t     alarm_system_is_active(void); // возвращает 1 если подсистема тревоги активна (т.е. в состоянии WARNING или ALARM)
as_state_t  alarm_system_get_state(void); // возвращает текущее состояние подсистемы тревоги