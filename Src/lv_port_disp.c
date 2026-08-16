#include "lv_port_disp.h"
#include "main.h"
#include <string.h>

/* Same SDRAM frame buffer address the picture-viewer's LCD_Config() mapped
   LCD layer 0 to (0xC0000000, see stm32f7508_discovery_lcd.h). ARGB8888,
   4 bytes/pixel, matching LV_COLOR_DEPTH 32 in lv_conf.h. RK043FN48H panel
   is 480x272. */
#define LTDC_FB_ADDRESS     ((uint8_t *)LCD_FB_START_ADDRESS)
#define LTDC_FB_WIDTH       480U
#define LTDC_BYTES_PER_PX   4U

void my_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
  int32_t w = lv_area_get_width(area);
  int32_t h = lv_area_get_height(area);
  uint32_t row_bytes = (uint32_t)w * LTDC_BYTES_PER_PX;

  uint8_t *dst_start = LTDC_FB_ADDRESS +
      (((uint32_t)area->y1 * LTDC_FB_WIDTH) + (uint32_t)area->x1) * LTDC_BYTES_PER_PX;
  uint8_t *dst = dst_start;
  uint8_t *src = px_map;

  for (int32_t row = 0; row < h; row++)
  {
    memcpy(dst, src, row_bytes);
    dst += (uint32_t)LTDC_FB_WIDTH * LTDC_BYTES_PER_PX;
    src += row_bytes;
  }

  /* SDRAM is MPU-configured cacheable (MPU_Config()) and the D-Cache is
     enabled (CPU_CACHE_Enable()), but the LTDC fetches the frame buffer
     directly from SDRAM, not through the CPU cache. Clean the written
     range so the panel doesn't read stale/partial pixels. */
  uint32_t clean_len = (uint32_t)(h - 1) * LTDC_FB_WIDTH * LTDC_BYTES_PER_PX + row_bytes;
  SCB_CleanDCache_by_Addr((uint32_t *)dst_start, (int32_t)clean_len);

  lv_display_flush_ready(disp);
}
