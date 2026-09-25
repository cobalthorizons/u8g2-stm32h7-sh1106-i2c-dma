/**
 * @file    u8g2_stm32h7xx_sh1106.h
 * @brief   SH1106 OLED display driver for STM32H7xx microcontrollers
 * @todo    prevent screen garbage at boot init
 * @author  J.M.Gaskill
 * @date    2024-06-05
 * @version 1.1.0
 * @note    This file is part of the CT50 Mk-I project.
 * @note    See the LICENSE file in the project root for license terms.
 */

#ifndef U8G2_STM32H7XX_SH1106_H
#define U8G2_STM32H7XX_SH1106_H

#include "u8g2.h"
#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ========================================================================== */
    /*                       Public Function Prototypes                          */
    /* ========================================================================== */

    /**
 * @brief  Prints low-level DMA, DMAMUX, and I2C status registers for debugging.
 *
 * This helper is intended for bring-up and fault isolation. It inspects the
 * STM32H7 DMA1 stream and I2C1 state to reveal NACKs, bus errors, DMA error
 * flags, TX DMA enable state, and DMAMUX routing. Use it when the panel fails
 * to initialize, the bus stalls, or a DMA transfer never completes.
 */
    void U8G2_HAL_Dump_I2C_DMA_State(void);

    /**
 * @brief  Prepares a fresh drawing frame and blocks until the previous DMA transfer is finished.
 *
 * This function clears the U8G2 drawing buffer only after confirming that the
 * prior I2C DMA transfer has completed. This avoids buffer reuse races during
 * frame updates and prevents stale data from being overwritten while the hardware
 * is still transmitting.
 */
    void U8G2_HAL_StartFrame(u8g2_t* u8g2);

    /**
 * @brief  Initializes the SH1106 panel and performs a clean startup clear.
 *
 * The routine configures the SH1106 backend, sets the panel address from
 * OLED_I2C_ADDRESS, sends the display initialization sequence, wakes the panel,
 * clears the framebuffer, pushes a blank frame to the display, and waits for the
 * initial DMA transfer to complete. This prevents stale SRAM contents and boot
 * artifacts from appearing on screen during system startup.
 */
    void U8G2_HAL_SH1106_Init(u8g2_t* u8g2, I2C_HandleTypeDef* hi2c);

    /* ========================================================================== */
    /*                       U8G2 Library Callback Bindings                       */
    /* ========================================================================== */

    /**
 * @brief  U8G2 byte-transfer callback for DMA-backed I2C transmission.
 *
 * This callback accumulates bytes into a DMA-safe staging buffer, waits for the
 * prior transfer to finish, and launches the SH1106 transaction through
 * HAL_I2C_Master_Transmit_DMA(). It is the core path for all display writes.
 */
    uint8_t u8x8_byte_stm32_hw_dma_i2c(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);

    /**
 * @brief  U8G2 GPIO and delay callback for STM32 timing and reset handling.
 *
 * This callback provides the timing primitives required by the U8G2 core, and
 * it is the correct place for reset and delay behavior when the hardware is
 * driven by STM32 HAL calls rather than a raw bit-banged transport.
 */
    uint8_t u8x8_gpio_and_delay_stm32(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);

#ifdef __cplusplus
}
#endif

#endif /* U8G2_STM32H7XX_HAL_H */