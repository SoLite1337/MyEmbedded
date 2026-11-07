#define THERMAL_TRAPS_H
#include <stdint.h>

// НАЗНАЧЕНИЕ:
// -  Подсистема «тепловых ловушек» (красный LED). Запускается по LONG-событию кнопки BTN_THERMAL.
//   Выполняет серию из 4 вспышек: 100 мс ON / 250 мс OFF.
// -  По завершении поднимает флаг burst_done, который читают другие
//   модули (например, alarm_system для выхода из тревоги).
// -  Реализовано как FSM (tt_state_t).
//

#define TT_ON_MS     100u   // длительность вспышки
#define TT_OFF_MS    250u   // пауза между вспышками
#define TT_COUNT     4u     // количество вспышек в серии

// Состояния FSM тепловых ловушек.
typedef enum 
{
    TT_ST_IDLE,       // ожидание LONG нажатия на BTN_THERMAL
    TT_ST_BURST_ON,   // текущая вспышка светит
    TT_ST_BURST_OFF,  // пауза между вспышками
} tt_state_t;

void thermal_traps_init(void); // инициализация (LED OFF, сброс флагов)
void thermal_traps_tick(void); // FSM тепловых ловушек, вызывается из system_tick()
uint8_t      thermal_traps_consume_burst_done(void); // есть ли флаг burst_done? Если есть, то сбрасываем его и ret 1, иначе ret 0.
uint8_t      thermal_traps_is_active(void);          // не IDLE?
tt_state_t   thermal_traps_get_state(void);          // получить текущее состояние FSM тепловых ловушек
