#include "event_bus.h"
#define EVENT_QUEUE_SIZE 32

// Очередь событий (кольцевой буфер)
static app_event_t s_queue[EVENT_QUEUE_SIZE];
static uint8_t s_head = 0;
static uint8_t s_tail = 0;


// Инициализация шины событий
void event_bus_init(void) 
{
    s_head = 0;
    s_tail = 0;
}

// Публикация события 
void event_bus_publish(app_event_t event) 
{
    // Добавление события в очередь
    uint8_t next_head = (s_head + 1) % EVENT_QUEUE_SIZE;

    if (next_head != s_tail) 
    { // Если очередь не переполнена
        s_queue[s_head] = event;
        s_head = next_head;
    }
}

// Чтение события из очереди
uint8_t event_bus_pop(app_event_t *out_event) 
{
    if (s_head == s_tail)
    {
        return 0; // Порожньо
    }
    // Извлечение события из очереди
    *out_event = s_queue[s_tail];
    s_tail = (s_tail + 1) % EVENT_QUEUE_SIZE;
    return 1;
}