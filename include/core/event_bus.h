#define EVENT_BUS_H
#include <stdint.h>

// Список всех событий, которые могут быть опубликованы в шину событий.
typedef enum 
{
    EVT_NONE = 0,
    
    // 1. События от кнопок (DL -> CL)
    EVT_BTN_THERMAL_SHORT,
    EVT_BTN_THERMAL_LONG,
    EVT_BTN_COMBAT_SHORT,
    EVT_BTN_COMBAT_LONG,
    EVT_BTN_SENSOR_SHORT,
    EVT_BTN_SENSOR_LONG,

    // 2. Системные события (CL -> CL / BL)
    EVT_THERMAL_BURST_DONE, // Тепловые ловушки завершили серию
} app_event_t;

void event_bus_init(void);
void event_bus_publish(app_event_t event);
uint8_t event_bus_pop(app_event_t *out_event); // Возвращает 1, если есть событие