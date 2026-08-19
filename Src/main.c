/**
  ******************************************************************************
  * @file    Display/LTDC_PicturesFromSDCard/Src/main.c
  * @author  MCD Application Team
  * @brief   This file provides main program functions
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
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/** @addtogroup STM32F7xx_HAL_Applications
  * @{
  */

/** @addtogroup LTDC_PicturesFromSDCard
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  uint8_t hr;
  uint8_t spo2;
} vitals_data_t;

/* Private define ------------------------------------------------------------*/
#define LV_DISP_HOR_RES   480U
#define LV_DISP_VER_RES   272U
#define LV_DRAW_BUF_LINES 40U   /* partial render buffer: 40 rows worth of pixels */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* LVGL partial-render draw buffer (LV_COLOR_DEPTH 32 -> 4 bytes/pixel).
   This is LVGL's software-render staging buffer, not the LTDC frame buffer:
   my_flush_cb() copies from here into the SDRAM frame buffer at LCD_FB_START_ADDRESS. */
static uint8_t lv_draw_buf[LV_DISP_HOR_RES * LV_DRAW_BUF_LINES * 4U];

/* Vitals queue: SensorSimTask produces, UITask consumes. Created in main()
   before either task starts, so both can reference it. */
static QueueHandle_t vitalsQueue;

/* SensorSimTask's PRNG state - a plain LCG, not newlib's rand()/srand().
   Seeded once via direct assignment in SensorSimTask, not srand(). */
static uint32_t prng_seed;

#if 0 /* BMP/FatFS: disabled */
FATFS SD_FatFs;  /* File system object for SD card logical drive */
char SD_Path[4]; /* SD card logical drive path */
char* pDirectoryFiles[MAX_BMP_FILES];
uint8_t  ubNumberOfFiles = 0;
uint32_t uwBmplen = 0;

/* Internal Buffer defined in SDRAM memory */
uint8_t *uwInternelBuffer;
#endif /* BMP/FatFS: disabled */

/* Private function prototypes -----------------------------------------------*/
static void MPU_Config(void);
static void LCD_Config(void);
static void SystemClock_Config(void);
static void Error_Handler(void);
static void CPU_CACHE_Enable(void);
#if 0 /* "Press me" button: no longer needed once touch was proven working */
static void btn_event_cb(lv_event_t * e);
#endif /* "Press me" button: no longer needed once touch was proven working */
static void UITask(void * pvParameters);
static void SensorSimTask(void * pvParameters);
static uint32_t simple_rand(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  BSP_LED_Init(LED1);


#if 0 /* BMP/FatFS: disabled */
  uint32_t counter = 0, transparency = 0;
  uint8_t str[30];
  uwInternelBuffer = (uint8_t *)0xC0260000;
#endif /* BMP/FatFS: disabled */

  /* Configure the MPU attributes */
  MPU_Config();

  /* Enable the CPU Cache */
  CPU_CACHE_Enable();

  /* STM32F7xx HAL library initialization:
       - Configure the Flash ART accelerator on ITCM interface
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
     */
  HAL_Init();
  
  /* Configure the system clock to 200 MHz */
  SystemClock_Config();
  
  /* Configure LED1 */
  BSP_LED_Init(LED1);
  
  /* Configure LCD */
  LCD_Config();

  /* Must exist before either task starts - both reference it. */
  vitalsQueue = xQueueCreate(5, sizeof(vitals_data_t));
  if (vitalsQueue == NULL)
  {
    /* Never checked before now. Distinct from the 5Hz task-creation-failure
       pattern below: three quick flashes then a pause, repeating, so a
       queue-creation failure reads as unmistakably different at a glance.
       HAL_Delay() is safe here, same reasoning as that check: HAL_Init() has
       already run and vTaskStartScheduler() hasn't been called yet, so
       SysTick_Handler's unconditional HAL_IncTick() keeps HAL_GetTick()
       advancing regardless of scheduler state. */
    for (;;)
    {
      for (uint8_t flash = 0; flash < 3; flash++)
      {
        BSP_LED_On(LED1);
        HAL_Delay(80);
        BSP_LED_Off(LED1);
        HAL_Delay(80);
      }

      HAL_Delay(600);
    }
  }

  /* All LVGL setup and the lv_timer_handler() pump now live in UITask -
     see below. 2048 words (8KB) is an estimate for LVGL widget
     creation/rendering headroom, not measured on hardware - tune if it
     turns out too tight or wastefully large. */
  xTaskCreate(UITask, "UITask", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);

  
  BaseType_t sensorTaskResult = xTaskCreate(SensorSimTask, "SensorSim", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
  if (sensorTaskResult != pdPASS)
  {
    /* Distinct from Error_Handler()'s steady-on LED and from
       vApplicationStackOverflowHook()'s interrupts-disabled busy-wait toggle:
       a fast, HAL_Delay()-timed blink at a known, measurable rate (5Hz).
       Runs here in main() before the scheduler starts, so HAL_Delay() is
       still safe - SysTick_Handler unconditionally calls HAL_IncTick()
       regardless of scheduler state. */
    for (;;)
    {
      BSP_LED_Toggle(LED1);
      HAL_Delay(100);
    }
  }

  /* TEMPORARY DIAGNOSTIC - remove once the heap budget (configTOTAL_HEAP_SIZE,
     Inc/FreeRTOSConfig.h) is settled. Reads the actual free-heap value off
     the hardware via LED blink count, no debugger needed: blinks once per
     free KB, clear pause between blinks, longer pause before repeating.
     This loops forever like the other diagnostic blocks above it, so it
     never reaches vTaskStartScheduler() below - boot halts here on purpose
     while this is in place. */
  size_t freeHeapBytes = xPortGetFreeHeapSize();
  uint32_t freeHeapKB = (uint32_t)((freeHeapBytes + 512U) / 1024U); /* round to nearest KB */

  vTaskStartScheduler();

  /* vTaskStartScheduler() only returns if there isn't enough heap left to
     create the idle/timer tasks - should never get here. */
  while (1)
  {
  }
}

/**
  * @brief  FreeRTOS UI task: owns all LVGL state end-to-end - lv_init(),
  *         display/touch driver registration, theme, widget creation, and
  *         the lv_timer_handler() pump. LVGL isn't thread-safe by default,
  *         so keeping every LVGL call confined to this one task avoids
  *         needing a mutex around it for now.
  * @param  pvParameters: unused
  * @retval None (never returns)
  */
static void UITask(void * pvParameters)
{
  (void)pvParameters;

  lv_init();

  /* HAL_GetTick() already increments every 1ms via SysTick_Handler/HAL_IncTick();
     hand it to LVGL directly instead of adding a second tick source. */
  lv_tick_set_cb(HAL_GetTick);

  lv_display_t *disp = lv_display_create(LV_DISP_HOR_RES, LV_DISP_VER_RES);
  lv_display_set_flush_cb(disp, my_flush_cb);
  lv_display_set_buffers(disp, lv_draw_buf, NULL, sizeof(lv_draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

  BSP_TS_Init(LV_DISP_HOR_RES, LV_DISP_VER_RES);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read_cb);
  lv_indev_set_display(indev, disp);

  /* v9 dropped the LV_THEME_DEFAULT_DARK macro from older versions - dark mode
     is now the `dark` bool param of lv_theme_default_init(). lv_theme_default_init()
     does not attach itself to the display, so lv_display_set_theme() is required too. */
  lv_color_t theme_color = lv_palette_main(LV_PALETTE_BLUE);
  lv_theme_t * theme = lv_theme_default_init(disp, theme_color, theme_color, true, LV_FONT_DEFAULT);
  lv_display_set_theme(disp, theme);

  /* Status banner: hardcoded green/NORMAL for now - Phase 5 wires this to
     actual fault detection. */
  lv_obj_t * status_banner = lv_obj_create(lv_screen_active());
  lv_obj_set_size(status_banner, LV_DISP_HOR_RES, 34);
  lv_obj_align(status_banner, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_radius(status_banner, 0, 0);
  lv_obj_set_style_bg_color(status_banner, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_set_style_bg_opa(status_banner, LV_OPA_COVER, 0);

  lv_obj_t * status_label = lv_label_create(status_banner);
  lv_label_set_text(status_label, LV_SYMBOL_WARNING " NORMAL");
  lv_obj_center(status_label);

  lv_obj_t * hr_label = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_font(hr_label, &lv_font_montserrat_48, 0);
  lv_label_set_text(hr_label, "72 BPM");
  lv_obj_align(hr_label, LV_ALIGN_TOP_MID, 0, 42);

  lv_obj_t * spo2_label = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_font(spo2_label, &lv_font_montserrat_48, 0);
  lv_label_set_text(spo2_label, "98%");
  lv_obj_align(spo2_label, LV_ALIGN_TOP_MID, 0, 100);

  /* Rolling-window trend chart: fed from vitalsQueue in the loop below,
     same as hr_label/spo2_label. Only one series exists, so it tracks HR
     (wider 60-100 range - more visually useful than SpO2's narrow 95-100
     band would be on a 0-100 y-axis). */
  lv_obj_t * chart = lv_chart_create(lv_screen_active());
  lv_obj_set_size(chart, 440, 95);
  lv_obj_align(chart, LV_ALIGN_TOP_MID, 0, 165);
  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
  lv_chart_set_point_count(chart, 30);
  lv_chart_series_t * chart_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

#if 0 /* "Press me" button: no longer needed once touch was proven working */
  /* v9 renamed lv_btn_create -> lv_button_create */
  lv_obj_t * btn = lv_button_create(lv_screen_active());
  lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -8);
  lv_obj_t * btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, "Press me");
  lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, btn_label);
#endif /* "Press me" button: no longer needed once touch was proven working */

  /* vTaskDelay(), not HAL_Delay(): this now runs as a scheduled task, so it
     should yield to other tasks between pumps rather than busy-spin. */
  for (;;)
  {
    vitals_data_t data;

    /* 0 timeout: a pure poll, never blocks. SensorSimTask only pushes once a
       second, so most iterations find nothing - that's expected, not an
       error. Must never stall lv_timer_handler(). */
    if (xQueueReceive(vitalsQueue, &data, 0) == pdPASS)
    {
      lv_label_set_text_fmt(hr_label, "%d BPM", data.hr);
      lv_label_set_text_fmt(spo2_label, "%d%%", data.spo2);
      lv_chart_set_next_value(chart, chart_series, data.hr);
    }

    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

#if 0 /* "Press me" button: no longer needed once touch was proven working */
/**
  * @brief  LV_EVENT_CLICKED handler for the "Press me" button: toggles the
  *         text of the label passed in as user_data between two strings.
  * @param  e: event descriptor (user_data holds the target label)
  * @retval None
  */
static void btn_event_cb(lv_event_t * e)
{
  static uint8_t pressed = 0;
  lv_obj_t * target_label = (lv_obj_t *)lv_event_get_user_data(e);

  pressed = !pressed;
  lv_label_set_text(target_label, pressed ? "Pressed!" : "Press me");
}
#endif /* "Press me" button: no longer needed once touch was proven working */

/**
  * @brief  Small linear congruential generator, standalone from newlib's
  *         rand()/srand(). Avoids relying on newlib's global rand() state
  *         (shared across all tasks even with configUSE_NEWLIB_REENTRANT,
  *         since rand()'s internal state isn't part of the per-task _reent
  *         struct) purely for SensorSimTask's own simulated noise - no
  *         other task touches prng_seed.
  * @param  None
  * @retval Pseudo-random value in [0, 0x7FFF]
  */
static uint32_t simple_rand(void)
{
  prng_seed = prng_seed * 1103515245U + 12345U;
  return (prng_seed >> 16) & 0x7FFF;
}

/**
  * @brief  FreeRTOS sensor-simulator task: generates HR (60-100 BPM) and
  *         SpO2 (95-100%) via a clamped random walk (small +-1/+-2 steps
  *         each cycle, not fully independent random jumps) so the values
  *         read as a plausible noisy signal rather than white noise.
  *         Pushes to vitalsQueue about once a second.
  * @param  pvParameters: unused
  * @retval None (never returns)
  */
static void SensorSimTask(void * pvParameters)
{
  vitals_data_t data;
  int8_t step;

  (void)pvParameters;

  data.hr = 75;
  data.spo2 = 98;
  prng_seed = HAL_GetTick(); /* plain assignment, not srand() */
  for (;;)
  {
    /* Loop-iteration sanity check: toggles unconditionally every pass,
       before any delay/queue logic, so it's visible proof the loop itself
       is actually repeating rather than stalling after one pass. */
    BSP_LED_Toggle(LED1);


    step = (int8_t)((simple_rand() % 5) - 2); /* -2..+2 */
    if ((data.hr + step) >= 60 && (data.hr + step) <= 100)
    {
      data.hr = (uint8_t)(data.hr + step);
    }

    step = (int8_t)((simple_rand() % 3) - 1); /* -1..+1 */
    if ((data.spo2 + step) >= 95 && (data.spo2 + step) <= 100)
    {
      data.spo2 = (uint8_t)(data.spo2 + step);
    }

    if (xQueueSend(vitalsQueue, &data, 0) == pdPASS)
    {
      /* Debug signal for a successful send - separate from every other LED
         usage in this project (Error_Handler()'s steady-on, the stack-overflow
         hook's interrupts-disabled busy-wait toggle, main()'s 5Hz
         task-creation-failure blink): a single brief flash, distinct from
         all three. Same LED1, since this board's BSP only exposes one. */
      BSP_LED_On(LED1);
      vTaskDelay(pdMS_TO_TICKS(20));
      BSP_LED_Off(LED1);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

/**
  * @brief  LCD configuration
  * @param  None
  * @retval None
  */
static void LCD_Config(void)
{
  /* LCD Initialization */ 
  BSP_LCD_Init();

  /* LCD Initialization */ 
  BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
  BSP_LCD_LayerDefaultInit(1, LCD_FB_START_ADDRESS+(BSP_LCD_GetXSize()*BSP_LCD_GetYSize()*4));

  /* Enable the LCD */ 
  BSP_LCD_DisplayOn(); 
  
  /* Select the LCD Background Layer  */
  BSP_LCD_SelectLayer(0);

  /* Clear the Background Layer */ 
  BSP_LCD_Clear(LCD_COLOR_BLACK);  
  
  /* Select the LCD Foreground Layer  */
  BSP_LCD_SelectLayer(1);

  /* Clear the Foreground Layer */ 
  BSP_LCD_Clear(LCD_COLOR_BLACK);
  
  /* Layer 0 is where my_flush_cb() writes LVGL's rendered pixels
     (LCD_FB_START_ADDRESS) so it must be opaque, not transparent as in the
     original slideshow (which ramped this up over time via BSP_LCD_SetTransparency
     in its cross-fade loop, now disabled). Layer 1 is unused, so keep it fully
     transparent instead of the original's partial value to avoid dimming Layer 0. */
  BSP_LCD_SetTransparency(0, 255);
  BSP_LCD_SetTransparency(1, 0);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* Turn LED1 on */
  BSP_LED_On(LED1);
  while(1)
  {
  }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 200000000
  *            HCLK(Hz)                       = 200000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 400
  *            PLL_P                          = 2
  *            PLL_Q                          = 8
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 6
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 400;  
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 8;

  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }

  /* Activate the OverDrive to reach the 200 MHz Frequency */
  ret = HAL_PWREx_EnableOverDrive();
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6);
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}


/**
  * @brief  Configure the MPU attributes
  * @param  None
  * @retval None
  */
static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU attributes for SDRAM */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0xC0000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_8MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER4;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */ 
  
