# Vital Signals

Simulated vitals-monitor HMI on the STM32F7508-DK, demonstrating safety-critical firmware patterns (deterministic UI rendering, cache-coherent DMA/framebuffer handling, layered build/init discipline) on bare-metal Cortex-M7.

## Status

**Phase 1 complete: LVGL rendering confirmed on hardware.**

This started as ST's stock `LTDC_PicturesFromSDCard` BMP-slideshow demo, converted from STM32CubeIDE to a standalone CMake build. The FatFS/BMP-decoding path has been stripped out (disabled, not deleted) and replaced with LVGL v9.6.0 driving the same LTDC/SDRAM framebuffer, rendering a basic label to confirm the display pipeline end-to-end.

## Target board

STM32F7508-DK (STM32F750N8H7, Cortex-M7 @ 200 MHz) — RK043FN48H 480x272 LCD panel over LTDC, SDRAM framebuffer, ARGB8888 / 32bpp.

## Toolchain

- VS Code + the STM32CubeIDE for VS Code extension (flashing/debugging)
- CMake (GCC ARM cross-compile), configured via `CMakeLists.txt` / `cmake/*.cmake`
- LVGL v9.6.0, vendored under `Middlewares/Third_Party/LVGL/`

## Architecture / demo

_Placeholder — architecture diagram and demo GIF land in Phase 6._

## License

Portions of this project are derived from STMicroelectronics' `LTDC_PicturesFromSDCard` example (see `readme.txt` and individual file headers) and are licensed under ST's terms. LVGL is MIT-licensed (see `Middlewares/Third_Party/LVGL/LICENCE.txt`).
