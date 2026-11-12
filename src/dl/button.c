#include "dl/button.h"
#include "dl/config.h"

// Инициализация кнопки, назначение пинов и событий
void button_init(button_t *b, GPIO_TypeDef *port, uint16_t pin, app_event_t evt_s, app_event_t evt_l) 
{
    b->port = port; b->pin = pin;
    b->state = BTN_ST_IDLE;
    b->timer = 0; b->saved_timer = 0;
    b->long_detected = 0; b->edge_irq = 0;
    b->evt_short = evt_s;
    b->evt_long = evt_l;
}

void button_irq(button_t *b) 
{ 
    b->edge_irq = 1; 
}

void button_tick(button_t *b) 
{
    switch (b->state) 
    {
        case BTN_ST_IDLE:
            if (b->edge_irq) 
            { 
                b->edge_irq = 0; b->state = BTN_ST_DEBOUNCE_PRESS; b->timer = BTN_DEBOUNCE_MS; 
            }
            break;

        case BTN_ST_DEBOUNCE_PRESS:
            if (b->edge_irq) 
            { 
                b->edge_irq = 0; b->timer = BTN_DEBOUNCE_MS; 
            }

            if (b->timer > 0 && --b->timer == 0) 
            {
                if (config_button_pressed(b->port, b->pin)) 
                { 
                    b->state = BTN_ST_HELD; b->long_detected = 0; b->timer = BTN_LONG_PRESS_MS - BTN_DEBOUNCE_MS; 
                }
                else 
                { 
                    b->state = BTN_ST_IDLE; 
                }
            }
            break;


        case BTN_ST_HELD:
            if (b->edge_irq) 
            { 
                b->edge_irq = 0; b->saved_timer = b->timer; b->state = BTN_ST_DEBOUNCE_RELEASE; b->timer = BTN_DEBOUNCE_MS; 
            }
            else if (b->timer > 0 && --b->timer == 0) 
            {
                b->long_detected = 1; b->state = BTN_ST_HELD_LONG;
                event_bus_publish(b->evt_long); // Публикация события длинного нажатия
            }
            break;


        case BTN_ST_HELD_LONG:
            if (b->edge_irq) 
            { 
                b->edge_irq = 0; b->state = BTN_ST_DEBOUNCE_RELEASE; b->timer = BTN_DEBOUNCE_MS; 
            }
            break;


        case BTN_ST_DEBOUNCE_RELEASE:
            if (b->edge_irq) 
            { 
                b->edge_irq = 0; b->timer = BTN_DEBOUNCE_MS; 
            }

            if (b->timer > 0 && --b->timer == 0) 
            {
                if (!config_button_pressed(b->port, b->pin)) 
                {
                    if (!b->long_detected) 
                    { 
                        event_bus_publish(b->evt_short);
                    } // Публикация события короткого нажатия
                    b->state = BTN_ST_IDLE; b->long_detected = 0;
                } 
                else 
                {
                    if (b->long_detected) 
                    { 
                        b->state = BTN_ST_HELD_LONG; 
                    }
                    else 
                    { 
                        b->state = BTN_ST_HELD; b->timer = b->saved_timer;
                    }
                }
            }
            break;
    }
}