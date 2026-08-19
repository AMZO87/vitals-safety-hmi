#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

/* configCHECK_FOR_STACK_OVERFLOW == 2 (Inc/FreeRTOSConfig.h) requires this to
   be implemented, or the link fails with an undefined reference. */

/**
  * @brief  FreeRTOS stack-overflow hook. Disables interrupts and toggles
  *         LED1 rapidly forever - a distinct pattern from Error_Handler()'s
  *         steady-on LED and from a plain frozen hang, so an overflow is
  *         immediately recognizable on hardware instead of looking like a
  *         generic lockup.
  * @param  xTask: handle of the task whose stack overflowed (unused)
  * @param  pcTaskName: name of the task whose stack overflowed (unused)
  * @retval None (never returns)
  */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  (void)xTask;
  (void)pcTaskName;

  /* Interrupts disabled below, so SysTick can no longer advance the HAL
     tick - HAL_Delay() would hang forever. A busy-wait loop is the only
     delay option left here. */
  taskDISABLE_INTERRUPTS();

  for (;;)
  {
    BSP_LED_Toggle(LED1);

    /* Approximate rapid-toggle rate, tuned by instruction count rather than
       measured on hardware - adjust if the actual blink rate looks off. */
    for (volatile uint32_t i = 0; i < 500000UL; i++)
    {
    }
  }
}
