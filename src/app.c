#include "app.h"
#include "core/event_bus.h"
#include "dl/config.h"
#include "dl/button.h"
#include "dl/led.h"
#include "cl/thermal_traps.h"
#include "cl/combat_system.h"
#include "cl/alarm_system.h"
#include "bl/system_state.h"

// Глобальные переменные для кнопок и светодиодов
button_t g_btn_thermal;
button_t g_btn_combat;
button_t g_btn_sensor;

led_t g_led_red;
led_t g_led_green;
led_t g_led_yellow;

// ==================================================================
void app_init(void) 
{
    config_init();
    event_bus_init(); // Ініціалізуємо шину

    // Dependency Injection: прив'язуємо фізичні кнопки до логічних подій
    button_init(&g_btn_thermal, BTN_THERMAL_PORT, BTN_THERMAL_PIN, EVT_BTN_THERMAL_SHORT, EVT_BTN_THERMAL_LONG);
    button_init(&g_btn_combat,  BTN_COMBAT_PORT,  BTN_COMBAT_PIN,  EVT_BTN_COMBAT_SHORT,  EVT_BTN_COMBAT_LONG);
    button_init(&g_btn_sensor,  BTN_SENSOR_PORT,  BTN_SENSOR_PIN,  EVT_BTN_SENSOR_SHORT,  EVT_BTN_SENSOR_LONG);

    led_init(&g_led_red,    LED_RED_PORT,    LED_RED_PIN,    0);
    led_init(&g_led_green,  LED_GREEN_PORT,  LED_GREEN_PIN,  0);
    led_init(&g_led_yellow, LED_YELLOW_PORT, LED_YELLOW_PIN, 0);

    thermal_traps_init();
    combat_system_init();
    alarm_system_init();
    system_state_init();
}

void app_tick(void) 
{
    // 1. DL: Оновлення фізичних станів (кнопки генерують події в шину)
    button_tick(&g_btn_thermal);
    button_tick(&g_btn_combat);
    button_tick(&g_btn_sensor);

    // 2. CL: FSM підсистем читають шину і виконують логіку
    thermal_traps_tick();
    combat_system_tick();
    alarm_system_tick();

    // 3. DL: Оновлення світлодіодів (виконують команди від CL)
    led_tick(&g_led_red);
    led_tick(&g_led_green);
    led_tick(&g_led_yellow);

    // 4. BL: Агрегація загального стану системи
    system_state_tick();
}