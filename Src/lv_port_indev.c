#include "lv_port_indev.h"
#include "main.h"

/* FT5336 already reports coordinates in panel pixel space (not a raw 0-4095
   range needing rescale): BSP_TS_GetState()'s I2cAddress == FT5336_I2C_SLAVE_ADDRESS
   branch (stm32f7508_discovery_ts.c) assigns touchX/touchY directly from the
   swapped-orientation reading, so no scaling is needed here. */
void my_touchpad_read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
  TS_StateTypeDef ts_state;

  BSP_TS_GetState(&ts_state);

  if (ts_state.touchDetected)
  {
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = ts_state.touchX[0];
    data->point.y = ts_state.touchY[0];
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}
