#ifndef __LV_PORT_DISP_H
#define __LV_PORT_DISP_H

#include "lvgl.h"

/* LVGL v9 flush callback: copies px_map into the LTDC SDRAM frame buffer
   at LCD_FB_START_ADDRESS and signals lv_display_flush_ready(). */
void my_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

#endif /* __LV_PORT_DISP_H */
