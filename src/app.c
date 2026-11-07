#include "app.h"
#include "config.h"
#include "button.h"
#include "led.h"
#include "thermal_traps.h"
#include "combat_system.h"
#include "alarm_system.h"
#include "system_state.h"

//
// Общие экземпляры
//

button_t g_btn_thermal;
button_t g_btn_combat;
button_t g_btn_sensor;

led_t g_led_red;
led_t g_led_green;
led_t g_led_yellow;

void app_init(void) //инициализация перефериии и подсистем
{
    config_init();                                 /* железо + TIM3 */

    button_init(&g_btn_thermal, BTN_THERMAL_PORT, BTN_THERMAL_PIN);
    button_init(&g_btn_combat,  BTN_COMBAT_PORT,  BTN_COMBAT_PIN);
    button_init(&g_btn_sensor,  BTN_SENSOR_PORT,  BTN_SENSOR_PIN);

    led_init(&g_led_red,    LED_RED_PORT,    LED_RED_PIN,    0);
    led_init(&g_led_green,  LED_GREEN_PORT,  LED_GREEN_PIN,  0);
    led_init(&g_led_yellow, LED_YELLOW_PORT, LED_YELLOW_PIN, 0);

    thermal_traps_init();
    combat_system_init();
    alarm_system_init();
    system_state_init();
}

// Такт системы. Вызывается из HAL_TIM_PeriodElapsedCallback каждую 1 мс.
//   1) сначала проходим по кнопкам и светодиодам, так как они генерируют события.
//   2) затем проходим по FSM-аа, так как они читают готовые события;
//   3) в конце обновляем агрегирующий system_state.

void app_tick(void)
{
    // Драйверы
    button_tick(&g_btn_thermal);
    button_tick(&g_btn_combat);
    button_tick(&g_btn_sensor);

    led_tick(&g_led_red);
    led_tick(&g_led_green);
    led_tick(&g_led_yellow);

    // 2. FSM.
    thermal_traps_tick();
    combat_system_tick();
    alarm_system_tick();

    // 3. States of System
    system_state_tick();
}