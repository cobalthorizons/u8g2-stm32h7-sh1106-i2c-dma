"# u8g2-stm32h7-sh1106-i2c-dma

A compact U8G2 driver layer for SH1106-based 128x64 OLED displays on STM32H7 microcontrollers using I2C with DMA. The project is designed to work as a thin hardware adapter between the U8G2 drawing API and the STM32 HAL/I2C DMA path, with buffering and completion tracking tuned for the H7 memory architecture.

## Overview

This repository provides:

- SH1106 128x64 OLED initialization via U8G2 setup helpers
- I2C transmit path accelerated through STM32 DMA
- A local DMA staging buffer placed in RAM_D2 for H7 compatibility
- A frame synchronization helper before rendering a new buffer
- Diagnostic register dump support for I2C and DMA debugging
- Host-side test stubs that exercise the driver logic without a target MCU

The driver is intentionally focused on the display transport layer. It assumes the U8G2 library and STM32Cube HAL environment already exist in the parent firmware project.

## File layout

- `src/u8g2_stm32h7xx_sh1106.h` — public API declarations and callback bindings
- `src/u8g2_stm32h7xx_sh1106.c` — SH1106 setup, DMA-backed I2C transfer, and diagnostics
- `tests/test_u8g2_stm32h7xx_sh1106.c` — host-side validation tests
- `tests/test_main.h` — minimal microcontroller register and HAL stubs used for host compilation

## Key features

- Uses `u8g2_Setup_sh1106_i2c_128x64_noname_f(...)` to configure the U8G2 SH1106 backend
- Sends display data via `HAL_I2C_Master_Transmit_DMA(...)`
- Tracks DMA completion with `i2c_dma_tx_complete`
- Waits for the previous transfer to finish before a new frame starts
- Keeps the transfer buffer in `.RAM_D2` and aligned to 32 bytes to avoid H7 DMA access issues in D2 memory
- Exposes a register-level diagnostics function for debugging DMA/I2C state

## Hardware assumptions

This driver is intended for:

- STM32H7 MCU family
- I2C OLED connection to SH1106 panel
- 128x64 display with a standard 7-bit I2C address, defaulted to `0x3C` in the code
- DMA-capable I2C peripheral configured for TX transfers

The code explicitly notes that STM32H7 DMA1/DMA2 cannot read from AXI SRAM in D1, so the staging buffer is placed in D2 memory (`RAM_D2` / SRAM3-style region) for compatibility.

## API

Public functions exposed by `u8g2_stm32h7xx_sh1106.h`:

- `void U8G2_HAL_StartFrame(u8g2_t* u8g2);`
  - waits for the previous DMA transfer to complete
  - clears the U8G2 drawing buffer before a new frame is rendered

- `void U8G2_HAL_SH1106_Init(u8g2_t* u8g2, I2C_HandleTypeDef* hi2c);`
  - stores the I2C handle
  - configures the U8G2 SH1106 backend
  - sets the device address
  - sends the display init sequence
  - wakes the panel and clears the framebuffer

- `void U8G2_HAL_Dump_I2C_DMA_State(void);`
  - prints DMA and I2C register state for debugging

U8G2 callback handlers:

- `uint8_t u8x8_byte_stm32_hw_dma_i2c(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);`
- `uint8_t u8x8_gpio_and_delay_stm32(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);`

## Typical usage

```c
#include "u8g2.h"
#include "u8g2_stm32h7xx_sh1106.h"

u8g2_t u8g2;

void InitDisplay(void)
{
    U8G2_HAL_SH1106_Init(&u8g2, &hi2c1);
}

void RenderFrame(void)
{
    U8G2_HAL_StartFrame(&u8g2);

    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
    u8g2_DrawStr(&u8g2, 0, 20, "HELLO");

    u8g2_SendBuffer(&u8g2);
}
```

This is the expected pattern: initialize once, then begin a frame, draw into the framebuffer, and transmit it to the display over DMA-backed I2C.

## Important implementation notes

### DMA buffer placement

The code defines a static staging buffer in `.RAM_D2` and aligns it to 32 bytes:

```c
__attribute__((section(".RAM_D2"), aligned(32)))
static uint8_t dma_buffer[256];
```

This is done to satisfy the STM32H7 DMA restrictions on AXI SRAM access and to improve coherent memory behavior for DMA transfers.

### Completion tracking

The driver uses a volatile flag:

```c
volatile uint8_t i2c_dma_tx_complete = 1;
```

This flag is set to 0 when DMA transmission begins and reset to 1 in the `HAL_I2C_MasterTxCpltCallback(...)` interrupt. This prevents overlapping frame writes while a prior DMA transfer is still in progress.

### Diagnostic usage

For debugging a stuck transfer or bus state, call:

```c
U8G2_HAL_Dump_I2C_DMA_State();
```

This prints low-level DMA and I2C register values to aid diagnosis of:

- DMA transfer errors
- NACKs or bus errors
- bus busy states
- TX DMA enable configuration
- DMAMUX selection for the I2C TX stream

## Project build and test notes

The repository includes a host-side test harness for validating the logic without running on hardware. It uses a mock register map and a minimal HAL stub layer in `tests/test_main.h`.

Run the tests in a normal C build environment with the test file and stub header included. The tests validate:

- buffer accumulation before DMA launch
- correct transmit size reporting
- successful completion flag handling
- diagnostic dump compilation against register-level stub values

## Dependencies

This project expects the following to be available in the embedded application:

- STM32Cube HAL for the target STM32H7 MCU
- U8G2 library
- A valid `main.h`/board initialization configuration that defines the I2C and GPIO setup used by the firmware project

## Notes

- The file `src/u8g2_stm32h7xx_sh1106.c` includes a static initialization command array for SH1106, but the main U8G2 setup function is the primary path used for initialization.
- The code is designed to be embedded into a larger firmware project rather than used as a standalone demo application.
- The `OLED_I2C_ADDRESS` constant is defined as `0x3C` in the host test stubs and in the library logic.

## License

This project is intended to be used with the project license terms available in the repository root. If no separate license file is present in your copy, check the parent project structure before publishing or redistributing the code.

## Summary

This repository provides a pragmatic STM32H7 + U8G2 + SH1106 adapter for DMA-based I2C OLED updates. It focuses on reliable DMA transmission, correct H7 memory placement, and low-level debug visibility without introducing excessive abstraction or complexity.
" 
