#include "stm32f4xx_hal.h"
#include "app.h"
#include "config.h"
#include "button.h"


extern TIM_HandleTypeDef htim3;

extern button_t g_btn_thermal;
extern button_t g_btn_combat;
extern button_t g_btn_sensor;

// Обработчики прерываний

void SysTick_Handler(void)
{
    HAL_IncTick();     // нужен HAL для таймаутов и задержек внутри HAL
}

void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim3);
}
void EXTI3_IRQHandler(void)  
{ 
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3); 
}
void EXTI4_IRQHandler(void)  
{ 
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4); 
}
void EXTI9_5_IRQHandler(void)
{ 
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5); 
}

void HardFault_Handler(void) 
{ 
    while (1) 
    {
        // Бесконечный цикл при ошибке
    } 
}

// Колбэки HAL 
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    switch (pin) 
    {
        case BTN_THERMAL_PIN: button_irq(&g_btn_thermal); break;
        case BTN_COMBAT_PIN:  button_irq(&g_btn_combat);  break;
        case BTN_SENSOR_PIN:  button_irq(&g_btn_sensor);  break;
        default: break;
    }
}

// TIM3: единый 1-мс такт всей системы.
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3) 
    {
        app_tick();
    }
}


int main(void)
{
    HAL_Init();
    app_init();

    while (1) 
    {
        __WFI();
    }
}