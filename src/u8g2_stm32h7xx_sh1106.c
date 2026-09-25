/**
 * @file    u8g2_stm32h7xx_sh1106.c
 * @brief   SH1106 OLED display driver for STM32H7xx microcontrollers using u8g2 library with I2C DMA support.
 * @todo    evaluate __DSB(); gold standard?
 * @author  J.M.Gaskill
 * @date    2026-08-28
 * @version 0.1.0
 * @note    #define OLED_I2C_ADDRESS ((uint16_t)(0x3C)) in main.h
 * @note    This file is part of the CT50 Mk-I project.
 * @note    See the LICENSE file in the project root for license terms.
 */

#include "u8g2_stm32h7xx_sh1106.h"
#include "main.h"
#include <stdio.h>

static I2C_HandleTypeDef* p_hi2c;  // Pointer to the I2C handle used for communication with the SH1106 display
// Flag to indicate the completion of the I2C DMA transmission. Set to 1 when the DMA transfer is complete, and 0 when a transfer is in progress.
volatile uint8_t i2c_dma_tx_complete = 1;
/* On the STM32H7, DMA1 and DMA2 physically cannot read from AXI SRAM (the D1 Domain).
    The AXI SRAM is in the D2 Domain, which is not accessible to DMA1 or DMA2.
    Therefore, we must place the buffer in the D2 Domain (RAM_D2, typically SRAM3 at 0x30040000) to allow DMA to read it.
    Additionally, we align the buffer to 32 bytes to match the cache line size of the H7,
    which helps avoid cache coherency issues when using DMA.
    160 bytes is 32-byte aligned (32 * 5). Perfect for Cortex-M7 cache lines. RAM_D2 is uncached */
__attribute__((
    section(".RAM_D2"),
    aligned(32))) static uint8_t dma_buffer[256];  // 160 bytes is more than enough for a single frame of SH1106 data
static uint16_t buf_idx = 0;                       // Index into the DMA buffer for the current transfer

/*  */
void U8G2_HAL_StartFrame(u8g2_t* u8g2)
{
    // Hardware Lock: Wait for ongoing DMA background transfers to finish before wiping the buffer
    while (!i2c_dma_tx_complete)
    {
        __NOP();
    }

    // Safe to clear the local frame canvas while the peripheral hardware sits idle
    u8g2_ClearBuffer(u8g2);
}

/* SH1106 Initialization Function */
void U8G2_HAL_SH1106_Init(u8g2_t* u8g2, I2C_HandleTypeDef* hi2c)
{
    p_hi2c = hi2c;  // Only map the real physical pointer on the actual chip

    /* Constructor for the SH1106 128x64 noname I2C hardware via u8g2_d_setup.c */
    u8g2_Setup_sh1106_i2c_128x64_noname_f(u8g2, U8G2_R1, u8x8_byte_stm32_hw_dma_i2c, u8x8_gpio_and_delay_stm32);
    u8g2_SetI2CAddress(u8g2, (OLED_I2C_ADDRESS << 1));  // Set the I2C address for the SH1106 display
    u8g2_InitDisplay(u8g2);                             // Send initialization sequence to the glass
    /* Comment the following line to prevent the display from being powered on  initialization */
    u8g2_SetPowerSave(u8g2, 0);  // Wake up display
    u8g2_ClearBuffer(u8g2);
    u8g2_SendBuffer(u8g2); // Push black frame to display
    while (!i2c_dma_tx_complete)
    {
        __NOP();
    }
}

/* Hardware byte transmission callback */
uint8_t u8x8_byte_stm32_hw_dma_i2c(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr)
{
    (void)u8x8;  // explicitly tell the compiler this is intentionally unused
    switch (msg)
    {
        case U8X8_MSG_BYTE_INIT:
            /* Initialization is handled in main.c via MX_I2C1_Init() */
            break;
        case U8X8_MSG_BYTE_SET_DC:
            /* ignored for standard hardware I2C protocols */
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            /* Force context synchronization. Wait for background DMA to clear RAM lines */
            while (!i2c_dma_tx_complete)
            {
                __NOP();
            }
            buf_idx = 0;
            break;
        case U8X8_MSG_BYTE_SEND:
        {
            uint8_t* data = (uint8_t*)arg_ptr;
            /* Guard against temporary array boundaries */
            while (arg_int > 0)
            {
                if (buf_idx < sizeof(dma_buffer))
                {
                    dma_buffer[buf_idx++] = *data;
                }
                data++;
                arg_int--;
            }
            break;
        }
        case U8X8_MSG_BYTE_END_TRANSFER:
        {
            if (buf_idx == 0) return 1;

            // Clear any lingering error states before starting DMA
            __HAL_I2C_CLEAR_FLAG(p_hi2c, I2C_FLAG_AF | I2C_FLAG_BERR | I2C_FLAG_OVR);

            // Treats short command bursts and pixel streams uniformly and transfer via DMA
            i2c_dma_tx_complete = 0;

            // FIX ME: should __DMB() and __DSB() be exclusively located in != HAL_OK block below?
            /* Use Data Synchronization Barriers to force CPU pipeline ordering
               Ensure the CPU store buffers are drained before returning */
            __DMB();  // Data Memory Barrier
            __DSB();  // Data Synchronization Barrier
            //__ISB();  // ???

            // Push local CPU cache to RAM_D2 so the physical DMA engine can read it
            if (HAL_I2C_Master_Transmit_DMA(p_hi2c, (OLED_I2C_ADDRESS << 1), dma_buffer, buf_idx) != HAL_OK)
            {
                i2c_dma_tx_complete = 1;  // Unlatch lock on structural failure
                /* Use Data Synchronization Barriers to force CPU pipeline ordering
                   Ensure the CPU store buffers are drained before returning */
                __DMB();  // Data Memory Barrier
                __DSB();  // Data Synchronization Barrier

                // Check the HAL Status (HAL_ERROR, HAL_BUSY, HAL_TIMEOUT)
                //volatile HAL_StatusTypeDef i2c_status = HAL_I2C_GetStatus(p_hi2c);

                // Check the hardware Error Code (e.g., 0x4 for AF / NACK)
                volatile uint32_t i2c_error = HAL_I2C_GetError(p_hi2c);

                // Check the peripheral state
                volatile HAL_I2C_StateTypeDef i2c_state = HAL_I2C_GetState(p_hi2c);

                printf("[I2C ERROR] Transmit Launch Failed. ErrorCode: 0x%lX, State: %d\n", i2c_error, (int)i2c_state);

                //HAL_GPIO_TogglePin(HEARTBEAT_LED_PORT, HEARTBEAT_LED_PIN);
                //HAL_Delay(50);
                return 0;
            }
            break;
        }

        default:
            return 0;
    }
    return 1;
}

/* Dummy/basic handler required for timing-based intervals within U8g2 core */
uint8_t u8x8_gpio_and_delay_stm32(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr)
{
    (void)u8x8;     // explicitly tell the compiler it is intentionally unused
    (void)arg_ptr;  // explicitly tell the compiler it is intentionally unused
    switch (msg)
    {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
/* GPIO initialization is handled in main.c via MX_GPIO_Init() */
#ifdef DEBUG
            printf("[U8G2 GPIO/DELAY] message: %d\n", msg);
#endif /* DEBUG */
            break;
        case U8X8_MSG_DELAY_MILLI:
            HAL_Delay(arg_int);
            HAL_Delay(1);  // Ensure at least 1ms delay for very short waits
            break;
        case U8X8_MSG_DELAY_10MICRO:  // 20260906 JMG: Added 10us delay for U8g2 core timing compliance
            for (uint32_t i = 0; i < arg_int * 100; i++)
            {
                __NOP();
            }  // Approximate for H7 @ 480MHz
            break;
        case U8X8_MSG_GPIO_RESET:
// U8g2 calls this to toggle the reset pin
// If you set U8X8_PIN_NONE, you safely do nothing here
// Assuming reset is handled elsewhere or tied to hardware reset
#ifdef DEBUG
            printf("[U8G2 GPIO/DELAY] message: %d\n", msg);
#endif /* DEBUG */
            break;
        default:
#ifdef DEBUG
            printf("[U8G2 GPIO/DELAY] Unhandled message: %d\n", msg);
#endif /* DEBUG */
            return 0;
    }
    return 1;
}

// This interrupt fires automatically whenever an internal I2C DMA completion occurs
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        // extern volatile uint8_t i2c_dma_tx_complete; // 20260906: Moved to main.c global scope for unified access
        // Only print in ISRs during active debugging, then comment it out.
        // printf("[I2C ISR SUCCESS] Transferred %d bytes\n", hi2c->XferSize);
        i2c_dma_tx_complete = 1;  // Release lock for next frame
    }
}

/**
  * @brief  Dumps the current state of I2C and DMA registers for diagnostic purposes.
  * @retval None
  */
void U8G2_HAL_Dump_I2C_DMA_State(void)
{
    printf("\n============ HARDWARE DIAGNOSTICS ============\n");

    // Check DMA1 Stream 0 Status Registers
    uint32_t dma_lisr = DMA1->LISR;  // Low Interrupt Status Register
    printf("[DMA1_S0] Raw LISR: 0x%08lX\n", dma_lisr);

    // Stream 0 flags are in bits 0-5 of LISR
    if (dma_lisr & (1U << 5)) printf("  -> [ERROR] Transfer Error (TEIF0) Active!\n");
    if (dma_lisr & (1U << 4)) printf("  -> [ERROR] FIFO Error (FEIF0) Active!\n");
    if (dma_lisr & (1U << 3)) printf("  -> [WARN] Direct Mode Error (DMEIF0) Active!\n");
    if (dma_lisr & (1U << 2)) printf("  -> Transfer Complete (TCIF0) Set.\n");

    printf("[DMA1_S0] Remaining NDTR (Bytes left to send): %ld\n", DMA1_Stream0->NDTR);
    printf("[DMA1_S0] Source Address (Memory): 0x%08lX\n", DMA1_Stream0->M0AR);
    printf("[DMA1_S0] Destination Address (I2C): 0x%08lX\n", DMA1_Stream0->PAR);

    // Check I2C1 Status Registers
    uint32_t i2c_isr = I2C1->ISR;  // Interrupt & Status Register
    uint32_t i2c_cr1 = I2C1->CR1;  // Control Register 1
    printf("[I2C1] Raw ISR: 0x%08lX\n", i2c_isr);
    printf("[I2C1] Raw CR1: 0x%08lX\n", i2c_cr1);

    if (i2c_isr & I2C_ISR_NACKF) printf("  -> [BUS] NACK Detected! Slave rejected address/data.\n");
    if (i2c_isr & I2C_ISR_BERR) printf("  -> [BUS] BUS ERROR! Misplaced Start/Stop condition on wire.\n");
    if (i2c_isr & I2C_ISR_OVR) printf("  -> [BUS] OVERRUN/UNDERRUN! Data wasn't supplied to shift register in time.\n");
    if (i2c_isr & I2C_ISR_BUSY) printf("  -> [BUS] Bus is locked (BUSY = 1).\n");

    if (!(i2c_cr1 & I2C_CR1_TXDMAEN))
    {
        printf("  -> [CONFIG ERROR] TXDMAEN bit is 0! I2C peripheral DMA mapping is turned OFF.\n");
    }

    // Check DMAMUX channel configuration for Stream 0
    // On H7, DMA1_Stream0 maps to DMAMUX1_Channel0
    uint32_t dmamux_ccr = DMAMUX1_Channel0->CCR;
    printf("[DMAMUX1_C0] Request ID Assigned: %ld (Should be 34 for I2C1_TX)\n", (dmamux_ccr & 0x7FU));

    printf("==============================================\n\n");
}

/* SH1106 Hardware Configuration Commands */
// Adding this attribute keeps the code compiled but mutes compiler warnings
static const uint8_t SH1106_Init_Commands[] __attribute__((unused)) = {
    0x00,  // I2C Control Byte: Following bytes are COMMANDS
    0xAE,  // 1. Display OFF (Sleep Mode)
    0x02,  // 2. Set Low Column Address (0x00 to 0x0F)
    0x10,  // 3. Set High Column Address (0x10 to 0x1F)
    0x40,  // 4. Set Display Start Line (0x40 to 0x7F)
    0xB0,  // 5. Set Page Address (0xB0 to 0xB7)
    0x81,  // 6. Set Contrast Control Mode
    0xCF,  //    Contrast Value (0x00 to 0xFF)
    0xA1,  // 7. Set Segment Re-map (A0=column 0->SEG0, A1=column 131->SEG0)
    0xA6,  // 8. Set Normal/Inverse Display (A6=Normal, A7=Inverse)
    0xA8,  // 9. Set Multiplex Ratio
    0x3F,  //    1/64 Duty
    0xAD,  // 10. Set DC-DC Charge Pump Converter
    0x8B,  //     0x8B = Enable Internal DC-DC (Crucial for display power!)
    0xC8,  // 11. Set COM Output Scan Direction (C0=Normal, C8=Remap)
    0xD3,  // 12. Set Display Offset
    0x00,  //     No Offset
    0xD5,  // 13. Set Display Clock Divide Ratio/Oscillator Frequency
    0x80,  //     Standard ratio
    0xD9,  // 14. Set Dis-charge/Pre-charge Period
    0x22,  //     Standard period
    0xDA,  // 15. Set COM Pins Hardware Configuration
    0x12,  //     Alternative configuration
    0xDB,  // 16. Set VCOMH Deselect Level
    0x40,  //     Output voltage level
    0xAF   // 17. DISPLAY ON! Wakes the panel up
};