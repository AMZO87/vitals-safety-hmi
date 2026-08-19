/**
  ******************************************************************************
  * @file    Display/LTDC_PicturesFromSDCard/Src/stm32f7xx_it.c 
  * @author  MCD Application Team
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f7xx_it.h"
#include "FreeRTOS.h"
#include "task.h"

/* Declared (not exported via any FreeRTOS public header) in
   Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1/port.c */
extern void xPortSysTickHandler(void);

/** @addtogroup STM32F7xx_HAL_Applications
  * @{
  */

/** @addtogroup LTDC_PicturesFromSDCard
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* SD handler declared in "stm32F7508_discovery_sd.c" file */
/* extern SD_HandleTypeDef uSdHandle; */
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M7 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  Shared fault-signal helper: blinks LED1 at ~2Hz forever. Called
  *         from HardFault_Handler() below and from Default_Handler() in
  *         startup_stm32f750xx.s (the fallback for every unhandled interrupt
  *         vector) - factored out into one function so both paths are
  *         guaranteed identical rather than two independently-tuned copies
  *         that could drift apart.
  *
  *         Deliberately calls BSP_LED_Init(LED1) every time, not just
  *         BSP_LED_Toggle(): Default_Handler can be hit very early (e.g. an
  *         unhandled interrupt before most init has run), so this cannot
  *         assume the GPIOI clock is enabled or the pin is already
  *         configured as an output. BSP_LED_Init() is idempotent - safe to
  *         call again even if normal init already ran.
  *
  *         Uses a manual busy-wait counter, NOT HAL_Delay()/vTaskDelay():
  *         tick/scheduler state may not be reliable during an actual fault,
  *         and may not exist at all yet if hit this early. A plain counting
  *         loop has no such dependency - though this does mean its real-world
  *         timing depends on whatever the CPU clock happens to be at the
  *         moment of the fault. The iteration count below is calibrated for
  *         the 200MHz post-SystemClock_Config() clock (deliberately slower
  *         than main()'s 5Hz task-creation-failure blink, so the two are
  *         easy to tell apart by eye): 200,000,000 cycles/sec x 0.5s target
  *         = 100,000,000 cycles needed; assuming ~10 CPU cycles per
  *         iteration for a -O0 volatile compare+increment+branch loop on
  *         Cortex-M7 (an estimate, not measured) gives 10,000,000
  *         iterations. If Default_Handler fires before SystemClock_Config()
  *         has run, the CPU is still on its slower reset clock, so the
  *         actual blink will be slower than 2Hz in that specific case - an
  *         inherent limit of cycle-counting without a clock reference, not
  *         a bug in the count itself.
  * @param  None
  * @retval None (never returns)
  */
void Fault_LED_Blink(void)
{
  BSP_LED_Init(LED1);

  for (;;)
  {
    BSP_LED_Toggle(LED1);

    for (volatile uint32_t i = 0; i < 10000000; i++)
    {
    }
  }
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  Fault_LED_Blink();
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/* SVC_Handler and PendSV_Handler are intentionally not defined here -
   FreeRTOS's port.c provides them via the vPortSVCHandler/xPortPendSVHandler
   #define mappings in FreeRTOSConfig.h. Neither fires until the scheduler
   explicitly triggers them, so there's no pre-scheduler-start hazard for
   these two the way there was for SysTick. */

/**
  * @brief  This function handles SysTick Handler.
  *         Always ticks the HAL (needed for HAL_GetTick()/HAL_Delay() during
  *         pre-scheduler init); only forwards to FreeRTOS's tick handler once
  *         the scheduler is actually running, since xPortSysTickHandler()
  *         touches kernel state (ready/delayed lists, pxCurrentTCB) that
  *         isn't initialized until vTaskStartScheduler() runs.
  *         FreeRTOSConfig.h deliberately does NOT map
  *         xPortSysTickHandler -> SysTick_Handler, so this is the only
  *         definition of SysTick_Handler in the project.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
  HAL_IncTick();

  if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
  {
    xPortSysTickHandler();
  }
}

/******************************************************************************/
/*                 STM32F7xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f7xx.s).                                               */
/******************************************************************************/
/**
  * @brief  This function handles SDMMC1 global interrupt request.
  * @param  None
  * @retval None
  */
void BSP_SDMMC_IRQHandler(void)
{
  /* HAL_SD_IRQHandler(&uSdHandle); */
}

/**
* @brief  This function handles DMA2 Stream 3 interrupt request.
* @param  None
* @retval None
*/
void BSP_SDMMC_DMA_Rx_IRQHandler(void)
{
  /* HAL_DMA_IRQHandler(uSdHandle.hdmarx); */
}

/**
  * @}
  */

/**
  * @}
  */ 


