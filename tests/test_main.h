#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

// 1. Core Compiler Attributes & Instruction Stubs
#define __attribute__(x)
#define __NOP() \
    do          \
    {           \
    } while (0)
#define __DMB() \
    do          \
    {           \
    } while (0)
#define __DSB() \
    do          \
    {           \
    } while (0)  // Resolves your Todo comment smoothly on host

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

// Global Pointer Symbols used in your driver file
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

#define __HAL_I2C_CLEAR_FLAG(handle, flags) \
    do                                      \
    {                                       \
        (void)handle;                       \
        (void)flags;                        \
    } while (0)

// 3. Complete Stubs for External Libraries (U8g2 Engine API)
typedef struct u8x8_struct u8x8_t;
typedef struct u8g2_struct
{
    uint8_t dummy;
} u8g2_t;
struct u8x8_struct
{
    uint8_t dummy;
};

#define U8G2_R1 1
#define U8X8_MSG_BYTE_INIT 1
#define U8X8_MSG_BYTE_SET_DC 2
#define U8X8_MSG_BYTE_START_TRANSFER 3
#define U8X8_MSG_BYTE_SEND 4
#define U8X8_MSG_BYTE_END_TRANSFER 5
#define U8X8_MSG_GPIO_AND_DELAY_INIT 10
#define U8X8_MSG_DELAY_MILLI 11
#define U8X8_MSG_DELAY_10MICRO 12
#define U8X8_PIN_NONE 14
#define U8X8_MSG_GPIO_RESET 13

inline void u8g2_ClearBuffer(u8g2_t* u) { (void)u; }
inline void u8g2_Setup_sh1106_i2c_128x64_noname_f(u8g2_t* u, const void* r,
                                                  uint8_t (*b)(u8x8_t*, uint8_t, uint8_t, void*),
                                                  uint8_t (*g)(u8x8_t*, uint8_t, uint8_t, void*))
{
    (void)u;
    (void)r;
    (void)b;
    (void)g;
}
inline void u8g2_SetI2CAddress(u8g2_t* u, uint8_t a)
{
    (void)u;
    (void)a;
}
inline void u8g2_InitDisplay(u8g2_t* u) { (void)u; }
inline void u8g2_SetPowerSave(u8g2_t* u, uint8_t e)
{
    (void)u;
    (void)e;
}

// Intercept Prototypes
HAL_StatusTypeDef HAL_I2C_Master_Transmit_DMA(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData,
                                              uint16_t Size);
uint32_t HAL_I2C_GetError(I2C_HandleTypeDef* hi2c);
HAL_I2C_StateTypeDef HAL_I2C_GetState(I2C_HandleTypeDef* hi2c);
void HAL_GPIO_TogglePin(void* port, uint16_t pin);
void HAL_Delay(uint32_t delay);
