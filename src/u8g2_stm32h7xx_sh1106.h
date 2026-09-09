/**
 * @file    u8g2_stm32h7xx_sh1106.h
 * @brief   SH1106 OLED display driver for STM32H7xx microcontrollers
 * @author  J.M.Gaskill
 * @date    2024-06-05
 * @version 1.0.0
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
 * @brief  Dumps the current state of I2C and DMA registers for diagnostics.
 */
    void U8G2_HAL_Dump_I2C_DMA_State(void);

    /**
 * @brief  Synchronizes and prepares the U8G2 context for a new drawing frame.
 */
    //void U8g2_StartFrame(u8g2_t *u8g2); // Hardware synchronization logic
    void U8G2_HAL_StartFrame(u8g2_t* u8g2);

    /**
 * @brief  Initializes the SH1106 OLED screen using the provided I2C handle.
 */
    //void U8g2_SH1106_Init(u8g2_t *u8g2, I2C_HandleTypeDef *hi2c); // Wrapper to initialize the display
    void U8G2_HAL_SH1106_Init(u8g2_t* u8g2, I2C_HandleTypeDef* hi2c);

    /* ========================================================================== */
    /*                       U8G2 Library Callback Bindings                       */
    /* ========================================================================== */

    uint8_t u8x8_byte_stm32_hw_dma_i2c(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);
    uint8_t u8x8_gpio_and_delay_stm32(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);

#ifdef __cplusplus
}
#endif

#endif /* U8G2_STM32H7XX_HAL_H */