/**
 * @file test_main.h
 * @brief Host-side hardware stubs and HAL mocking structures for STM32H7xx.
 */
#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// 1. Safe Microcontroller Instruction Stubs
#ifndef __NOP
#define __NOP() \
    do          \
    {           \
    } while (0)
#endif
#ifndef __DMB
#define __DMB() \
    do          \
    {           \
    } while (0)
#endif
#ifndef __DSB
#define __DSB() \
    do          \
    {           \
    } while (0)
#endif

// 2. Exact Memory-Mapped Microcontroller Register Layout Stubs
typedef struct
{
    volatile uint32_t LISR;
    volatile uint32_t HISR;
    volatile uint32_t LIFCR;
    volatile uint32_t HIFCR;
} DMA_TypeDef;

typedef struct
{
    volatile uint32_t CR;
    volatile uint32_t NDTR;
    volatile uint32_t PAR;
    volatile uint32_t M0AR;
    volatile uint32_t M1AR;
    volatile uint32_t FCR;
} DMA_Stream_TypeDef;

typedef struct
{
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t TIMINGR;
    volatile uint32_t TIMEOUTR;
    volatile uint32_t ISR;
    volatile uint32_t ICR;
    volatile uint32_t PECR;
    volatile uint32_t RXDR;
    volatile uint32_t TXDR;
} I2C_TypeDef;

typedef struct
{
    volatile uint32_t CCR;
} DMAMUX_Channel_TypeDef;

// Global Pointer Symbols exported to the driver
extern DMA_TypeDef* DMA1;
extern DMA_Stream_TypeDef* DMA1_Stream0;
extern I2C_TypeDef* I2C1;
extern DMAMUX_Channel_TypeDef* DMAMUX1_Channel0;

// Hardware Constants Matching STM32 HAL Library
#define I2C_FLAG_AF (1U << 0)
#define I2C_FLAG_BERR (1U << 1)
#define I2C_FLAG_OVR (1U << 2)
#define I2C_ISR_NACKF (1U << 4)
#define I2C_ISR_BERR (1U << 5)
#define I2C_ISR_OVR (1U << 6)
#define I2C_ISR_BUSY (1U << 7)
#define I2C_CR1_TXDMAEN (1U << 0)
#define OLED_I2C_ADDRESS 0x3C

#define HEARTBEAT_LED_PORT ((void*)0x1)
#define HEARTBEAT_LED_PIN ((uint16_t)0x0001)

typedef enum
{
    HAL_OK = 0x00U,
    HAL_ERROR = 0x01U
} HAL_StatusTypeDef;

typedef uint32_t HAL_I2C_StateTypeDef;

typedef struct
{
    I2C_TypeDef* Instance;
    uint32_t XferSize;
} I2C_HandleTypeDef;

#define __HAL_I2C_CLEAR_FLAG(handle, flags) ((void)0)

// Mock Control Interface (Allows tests to dynamically inject failures)
void mock_hal_reset(void);
void mock_hal_set_transmit_status(HAL_StatusTypeDef status);
int mock_hal_get_transmit_count(void);
uint16_t mock_hal_get_last_tx_size(void);
const uint8_t* mock_hal_get_captured_buffer(void);

// Intercept Prototypes
HAL_StatusTypeDef HAL_I2C_Master_Transmit_DMA(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData,
                                              uint16_t Size);
uint32_t HAL_I2C_GetError(I2C_HandleTypeDef* hi2c);
HAL_I2C_StateTypeDef HAL_I2C_GetState(I2C_HandleTypeDef* hi2c);
void HAL_GPIO_TogglePin(void* port, uint16_t pin);
void HAL_Delay(uint32_t delay);
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* hi2c);
