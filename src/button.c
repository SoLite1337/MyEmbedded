#include "button.h"
#include "config.h"

//  НАЗНАЧЕНИЕ:
//   Драйвер кнопки с поддержкой:
//    - антидребезга (debounce);
//    - короткого нажатия (short press);
//    - длинного нажатия (long press).
//   Реализован как FSM с состояниями button_state_t.
//   Внешний код вызывает button_tick() раз в 1 мс, а button_irq() вызывается из HAL_GPIO_EXTI_Callback().


void button_init(button_t *b, GPIO_TypeDef *port, uint16_t pin) // инициализация
{
    b->port          = port;            // порт
    b->pin           = pin;             // пин
    b->state         = BTN_ST_IDLE;     // состояние FSM
    b->timer         = 0;               // знач. таймера текущее
    b->saved_timer   = 0;               // знач таймера сохраненное
    b->long_detected = 0;               // 1, если LONG уже выдан (чтобы при отпускание не получили SHORT)
    b->edge_irq      = 0;               // флаг фронта, ставится в EXTI ISR
    b->pending       = BTN_EVT_NONE;    // событие наружу
}

// Вызывается из HAL_GPIO_EXTI_Callback и ставит флаг.
void button_irq(button_t *b)
{
    b->edge_irq = 1;
}

// Шаг FSM. Вызывается раз в 1 мс из app_tick().
void button_tick(button_t *b)
{
    switch (b->state) 
    {

        case BTN_ST_IDLE:
            //начало нажатия
            if (b->edge_irq) 
            {
                b->edge_irq = 0;
                b->state    = BTN_ST_DEBOUNCE_PRESS;
                b->timer    = BTN_DEBOUNCE_MS;
            }
            break;

        case BTN_ST_DEBOUNCE_PRESS:
            // Новый фронт = дребезг -> перезапуск таймера.
            if (b->edge_irq) 
            {
                b->edge_irq = 0;
                b->timer    = BTN_DEBOUNCE_MS;
            }

            if (b->timer > 0 && --b->timer == 0) 
            {
                if (config_button_pressed(b->port, b->pin)) 
                {
                    // Дребезг прошел, фикисируем нажатие.
                    b->state         = BTN_ST_HELD;
                    b->long_detected = 0;
                    b->timer         = BTN_LONG_PRESS_MS - BTN_DEBOUNCE_MS;
                } 
                else 
                {
                    // Ложное срабатывание - IDLE.
                    b->state = BTN_ST_IDLE;
                }
            }
            break;

        case BTN_ST_HELD:
            if (b->edge_irq) 
            {
                // Возможное отпускание - проверяем дебаунс релиза, запомнив, сколько ещё осталось до LONG.
                b->edge_irq    = 0;
                b->saved_timer = b->timer;
                b->state       = BTN_ST_DEBOUNCE_RELEASE;
                b->timer       = BTN_DEBOUNCE_MS;
            } 
            else if (b->timer > 0 && --b->timer == 0) 
            {
                // Порог длинного нажатия достигнут.
                b->long_detected = 1;
                b->state         = BTN_ST_HELD_LONG;
                b->pending       = BTN_EVT_LONG_PRESS;
            }
            break;

        case BTN_ST_HELD_LONG:
            // Ждем отпускания, событий не генерируем. */
            if (b->edge_irq) 
            {
                b->edge_irq = 0;
                b->state    = BTN_ST_DEBOUNCE_RELEASE;
                b->timer    = BTN_DEBOUNCE_MS;
            }
            break;

        case BTN_ST_DEBOUNCE_RELEASE:
            // Дребезг при отпускании - перезапуск таймера.
            if (b->edge_irq) 
            {
                b->edge_irq = 0;
                b->timer    = BTN_DEBOUNCE_MS;
            }
            if (b->timer > 0 && --b->timer == 0) 
            {
                if (!config_button_pressed(b->port, b->pin)) 
                {
                    // Если реально отпущена
                    if (!b->long_detected) 
                    {
                        b->pending = BTN_EVT_SHORT_PRESS;
                    }
                    b->state         = BTN_ST_IDLE;
                    b->long_detected = 0;
                } 
                else 
                {
                    // НО если ложное отпускание, то вернуться в исходное состояние + восстановив таймер.
                    if (b->long_detected) 
                    {
                        b->state = BTN_ST_HELD_LONG;
                    } 
                    else 
                    {
                        b->state = BTN_ST_HELD;
                        b->timer = b->saved_timer;
                    }
                }
            }
            break;
    }
}



btn_event_t button_get_event(button_t *b) // получить событие, если есть. Сбрасывает pending.
{
    btn_event_t e = b->pending; //знач. в локальную переменную
    b->pending = BTN_EVT_NONE;  // сбрасываем
    return e;                   // возвращаем значение
}

//идентично ополучаем состояние кнопки, но без сброса состояния
btn_state_t button_get_state(const button_t *b)
{
    return b->state;
}