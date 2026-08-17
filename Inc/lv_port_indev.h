#ifndef __LV_PORT_INDEV_H
#define __LV_PORT_INDEV_H

#include "lvgl.h"

/* LVGL v9 indev read callback: polls the FT5336 touch controller via the
   BSP touch wrapper (BSP_TS_GetState) and reports a single touch point. */
void my_touchpad_read_cb(lv_indev_t * indev, lv_indev_data_t * data);

#endif /* __LV_PORT_INDEV_H */
