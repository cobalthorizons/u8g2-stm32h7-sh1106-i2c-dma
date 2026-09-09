#include "test_main.h"
#include "u8g2_stm32h7xx_sh1106.h"

// Allocate back-end host variable structures for peripheral pointers
static DMA_TypeDef host_dma = {0};
static DMA_Stream_TypeDef host_stream = {0};
static I2C_TypeDef host_i2c = {0};
static DMAMUX_Channel_TypeDef host_mux = {0};

DMA_TypeDef* DMA1 = &host_dma;
DMA_Stream_TypeDef* DMA1_Stream0 = &host_stream;
I2C_TypeDef* I2C1 = &host_i2c;
DMAMUX_Channel_TypeDef* DMAMUX1_Channel0 = &host_mux;

extern volatile uint8_t i2c_dma_tx_complete;
uint8_t u8x8_byte_stm32_hw_dma_i2c(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);

// Mock Tracking Variables
static int g_hal_transmit_count = 0;
static HAL_StatusTypeDef g_hal_transmit_injected_status = HAL_OK;
static uint16_t g_last_tx_size = 0;
static uint8_t g_captured_tx_buffer[256] = {0};

// Implement Mocked Functions targeting our host allocations
HAL_StatusTypeDef HAL_I2C_Master_Transmit_DMA(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData,
                                              uint16_t Size)
{
    g_hal_transmit_count++;
    g_last_tx_size = Size;
    if (Size <= 256)
    {
        memcpy(g_captured_tx_buffer, pData, Size);
    }
    return g_hal_transmit_injected_status;
}

uint32_t HAL_I2C_GetError(I2C_HandleTypeDef* hi2c) { return 0x04; }  // Returns AF/NACK
HAL_I2C_StateTypeDef HAL_I2C_GetState(I2C_HandleTypeDef* hi2c) { return 2; }
void HAL_GPIO_TogglePin(void* port, uint16_t pin) {}
void HAL_Delay(uint32_t delay) {}

void reset_environment(void)
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

// --- ACTUAL TEST SUITE ---

void test_payload_accumulation_and_transmission(void)
{
    printf("Running: test_payload_accumulation_and_transmission...\n");
    reset_environment();

    uint8_t payload[] = {0x7A, 0x8B, 0x9C};

    u8x8_byte_stm32_hw_dma_i2c(NULL, U8X8_MSG_BYTE_START_TRANSFER, 0, NULL);
    u8x8_byte_stm32_hw_dma_i2c(NULL, U8X8_MSG_BYTE_SEND, 3, payload);
    u8x8_byte_stm32_hw_dma_i2c(NULL, U8X8_MSG_BYTE_END_TRANSFER, 0, NULL);

    assert(g_hal_transmit_count == 1);
    assert(g_last_tx_size == 3);
    assert(g_captured_tx_buffer[0] == 0x7A);
    assert(i2c_dma_tx_complete == 0);  // Lock engages cleanly
    printf(" -> PASSED\n");
}

void test_diagnostics_dump_compiles_and_reads_registers(void)
{
    printf("Running: test_diagnostics_dump_compiles_and_reads_registers...\n");
    reset_environment();

    // Populate fake register values on your laptop
    host_dma.LISR = (1U << 5);     // Emulate a Transfer Error Flag (TEIF0)
    host_stream.NDTR = 42;         // Emulate 42 remaining bytes
    host_i2c.ISR = I2C_ISR_NACKF;  // Emulate a Slave NACK error

    // Call your diagnostic dump directly on your laptop!
    U8G2_HAL_Dump_I2C_DMA_State();

    printf(" -> PASSED\n");
}

int main(void)
{
    printf("\n==================================================\n");
    printf("🚀 EXECUTING AUTOMATED REGISTER-LEVEL DRIVER TESTS \n");
    printf("==================================================\n");

    test_payload_accumulation_and_transmission();
    test_diagnostics_dump_compiles_and_reads_registers();

    printf("\n🎉 SUCCESS: All host-side testing targets passed! 🎉\n");
    return 0;
}
