/**
 * @file test_u8g2_stm32h7xx_sh1106.c
 * @brief Unit tests for SH1106 display driver utilizing the Unity Framework.
 */
#include "unity.h"
#include "test_main.h"
#include "u8g2_stm32h7xx_sh1106.h"

// Allocate back-end host variable structures for peripheral pointers
static DMA_TypeDef host_dma;
static DMA_Stream_TypeDef host_stream;
static I2C_TypeDef host_i2c;
static DMAMUX_Channel_TypeDef host_mux;

DMA_TypeDef* DMA1 = &host_dma;
DMA_Stream_TypeDef* DMA1_Stream0 = &host_stream;
I2C_TypeDef* I2C1 = &host_i2c;
DMAMUX_Channel_TypeDef* DMAMUX1_Channel0 = &host_mux;

// Driver-under-test shared state variables
extern volatile uint8_t i2c_dma_tx_complete;
uint8_t u8x8_byte_stm32_hw_dma_i2c(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);

// Mock State Variables
static int g_hal_transmit_count = 0;
static HAL_StatusTypeDef g_hal_transmit_injected_status = HAL_OK;
static uint16_t g_last_tx_size = 0;
static uint8_t g_captured_tx_buffer[256] = {0};

// --- MOCK INTERFACE IMPLEMENTATIONS ---
void mock_hal_reset(void)
{
    i2c_dma_tx_complete = 1;
    g_hal_transmit_count = 0;
    g_hal_transmit_injected_status = HAL_OK;
    g_last_tx_size = 0;
    memset(g_captured_tx_buffer, 0, sizeof(g_captured_tx_buffer));
    memset(&host_dma, 0, sizeof(host_dma));
    memset(&host_stream, 0, sizeof(host_stream));
    memset(&host_i2c, 0, sizeof(host_i2c));
    memset(&host_mux, 0, sizeof(host_mux));
}

void mock_hal_set_transmit_status(HAL_StatusTypeDef status) { g_hal_transmit_injected_status = status; }
int mock_hal_get_transmit_count(void) { return g_hal_transmit_count; }
uint16_t mock_hal_get_last_tx_size(void) { return g_last_tx_size; }
const uint8_t* mock_hal_get_captured_buffer(void) { return g_captured_tx_buffer; }

HAL_StatusTypeDef HAL_I2C_Master_Transmit_DMA(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData,
                                              uint16_t Size)
{
    (void)hi2c;
    (void)DevAddress;
    g_hal_transmit_count++;
    g_last_tx_size = Size;
    if (Size <= 256)
    {
        memcpy(g_captured_tx_buffer, pData, Size);
    }
    return g_hal_transmit_injected_status;
}

uint32_t HAL_I2C_GetError(I2C_HandleTypeDef* hi2c)
{
    (void)hi2c;
    return 0x04;
}
HAL_I2C_StateTypeDef HAL_I2C_GetState(I2C_HandleTypeDef* hi2c)
{
    (void)hi2c;
    return 2;
}
void HAL_GPIO_TogglePin(void* port, uint16_t pin)
{
    (void)port;
    (void)pin;
}
void HAL_Delay(uint32_t delay) { (void)delay; }

// --- UNITY HOOKS ---
void setUp(void) { mock_hal_reset(); }

void tearDown(void)
{
    // Left intentional for future tear downs (e.g. freeing memory)
}

// --- ACTUAL TEST SUITE ---

void test_payload_accumulation_and_transmission(void)
{
    uint8_t payload[] = {0x7A, 0x8B, 0x9C};

    u8x8_byte_stm32_hw_dma_i2c(NULL, U8X8_MSG_BYTE_START_TRANSFER, 0, NULL);
    u8x8_byte_stm32_hw_dma_i2c(NULL, U8X8_MSG_BYTE_SEND, 3, payload);
    u8x8_byte_stm32_hw_dma_i2c(NULL, U8X8_MSG_BYTE_END_TRANSFER, 0, NULL);

    TEST_ASSERT_EQUAL_INT(1, mock_hal_get_transmit_count());
    TEST_ASSERT_EQUAL_UINT16(3, mock_hal_get_last_tx_size());
    TEST_ASSERT_EQUAL_HEX8(0x7A, mock_hal_get_captured_buffer()[0]);
    TEST_ASSERT_EQUAL_UINT8(0, i2c_dma_tx_complete);
}

void test_diagnostics_dump_compiles_and_reads_registers(void)
{
    host_dma.LISR = (1U << 5);     // Emulate a Transfer Error Flag (TEIF0)
    host_stream.NDTR = 42;         // Emulate 42 remaining bytes
    host_i2c.ISR = I2C_ISR_NACKF;  // Emulate a Slave NACK error

    // Verifies that calling diagnostics doesn't cause segmentation faults or host crashes
    U8G2_HAL_Dump_I2C_DMA_State();
}

void test_dma_interrupt_unlatches_driver_lock(void)
{
    uint8_t frame_data[] = {0x00, 0xAA, 0xBB};
    I2C_HandleTypeDef fake_handle = {.Instance = &host_i2c};

    u8x8_byte_stm32_hw_dma_i2c(NULL, U8X8_MSG_BYTE_START_TRANSFER, 0, NULL);
    u8x8_byte_stm32_hw_dma_i2c(NULL, U8X8_MSG_BYTE_SEND, 3, frame_data);
    u8x8_byte_stm32_hw_dma_i2c(NULL, U8X8_MSG_BYTE_END_TRANSFER, 0, NULL);

    TEST_ASSERT_EQUAL_UINT8(0, i2c_dma_tx_complete);

    // Call the true callback handled by the driver submodule
    HAL_I2C_MasterTxCpltCallback(&fake_handle);

    TEST_ASSERT_EQUAL_UINT8(1, i2c_dma_tx_complete);
}

#include "unity.h"
#include "u8g2.h"
// Include your driver header file here (e.g., "u8g2_stm32h7xx_sh1106.h")

// --- Assume these match your mock environment's internal register states ---
extern uint32_t MOCK_I2C1_CR1;
extern uint32_t MOCK_DMAMUX1_C0_CR;
extern uint32_t MOCK_DMA1_S0_CR;

// --- TEST RUNNER ENGINE ---
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_payload_accumulation_and_transmission);
    RUN_TEST(test_diagnostics_dump_compiles_and_reads_registers);
    RUN_TEST(test_dma_interrupt_unlatches_driver_lock);
    return UNITY_END();
}
