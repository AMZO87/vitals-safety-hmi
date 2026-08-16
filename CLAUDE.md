# Project: LTDC_PicturesFromSDCard → LVGL Bring-up

## Overview
Portfolio project on the STM32F7508-DK (Discovery kit). Starting from ST's
`LTDC_PicturesFromSDCard` example (proven working LTDC/SDRAM/touch init +
SD card picture viewer), the goal is to strip out the picture-viewer logic
and bring up LVGL on the same display pipeline. End state for Phase 1: a
static LVGL screen ("Hello" label) rendering correctly on hardware.

## Target
- **Board**: STM32F7508-DK (also referred to as STM32F7508-DISCO/DISCOVERY
  across ST's tooling — same board)
- **MCU**: STM32F750N8H6
- **Display**: 480x272 via LTDC, frame buffer in external SDRAM
- **Boot**: flashed to QSPI external flash (XIP), same process as the
  original picture-viewer demo

## Build
- Toolchain: arm-none-eabi-gcc (via STM32CubeIDE for VS Code's bundle
  manager), CMake + Ninja generator
- Configure: `cmake --preset Debug`
- Build: `cmake --build build/Debug`
- Output: `build/Debug/<target>.elf` (adjust path if actual preset/binaryDir
  differs — check CMakePresets.json)
- **Debug flags matter**: `cmake/flags.cmake` must have `-O0 -g` for Debug
  (was previously misconfigured to `-Os`, which silently broke debug
  symbols and breakpoint binding)

## Provenance / how this project was set up
Converted from ST's original SW4STM32-format project via desktop
STM32CubeIDE (File → Open Projects from File System, which auto-migrates
legacy SW4STM32 projects to native CubeIDE format), then converted again
to CMake using VS Code's "Convert Eclipse STM32CubeIDE project" command.
Confirmed compiling and flashing correctly before any LVGL work began.

## Do not touch (proven working — treat as a stable foundation)
- LTDC peripheral init and timing configuration
- SDRAM init (FMC configuration)
- Touch controller init
- Anything under `Drivers/BSP/STM32F7508-Discovery/` unless specifically
  required for LVGL flush/touch callback wiring

## Known issue — debugging
VS Code's integrated debugger (STM32CubeIDE for VS Code extension) has been
unreliable on this project: breakpoints not binding, call stack/source not
resolving even after fixing debug symbol flags. Root cause not yet
confirmed (possibly leftover background GDB server / probe session state).
**Until this is resolved**: verify behavior via visible output (LED toggle,
UART print, or actual screen content) rather than relying on breakpoints.
Desktop STM32CubeIDE's debugger is a working fallback if step-through
debugging becomes necessary.

## Phase 1 plan — LVGL bring-up
1. Clone LVGL (`github.com/lvgl/lvgl`) and a reference port
   (`lv_port_stm32f746_disco` or `STM32F7508-DK-LVGL-AzureRtos-Template`) —
   **read only**, to see how display-flush and touch-read callbacks are
   wired. Do not copy these projects wholesale.
2. Identify and list every BMP/FatFS-specific file and function in this
   project before removing anything — present the list for review first.
3. Strip the BMP/FatFS logic once approved. Keep LTDC/SDRAM/touch init
   untouched (see "Do not touch" above).
4. Add LVGL source files to the project; add `lv_conf.h` (from LVGL's
   template), set resolution to 480x272.
5. Write the LVGL display-flush function: copies LVGL's internal render
   buffer into the same SDRAM frame buffer address the picture demo used.
6. Write a minimal `main()`: `lv_init()`, register the display driver,
   create one `lv_label` saying "Hello", call `lv_timer_handler()` in a
   loop.
7. Build, flash to QSPI at `0x90000000` (same process as the original
   demo), reset, confirm the label renders on the physical screen.

## Working style
- Build after every meaningful change (`cmake --build build/Debug`) —
  don't batch up multiple untested changes.
- Before deleting or rewriting existing working code, summarize what will
  change and wait for confirmation.
- This is portfolio work — prefer clean, well-commented code and sensible
  commit granularity over speed.
