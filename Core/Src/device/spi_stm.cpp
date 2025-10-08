#ifdef __cplusplus
extern "C" {
#endif


// SPI functions for communication with FPGA and implementation of MCU-FPGA protocol operations
// MCU is configured as quad SPI master
// The FPGA expects commands in a specific format
// the first nibble is the OPCODE and the following varies by the operation being sent
// the FPGA will respond with a single nibble return code followed by up to one byte of data
// depending on the operation

// constants for protocol commands
// OPCODES 5 -> 15 is available for more commands
// Return codes 5 -> 15 is available for more return codes

#include <cstdint>
#include <cstring>
#include "main.h"


// Operations:
enum {
    WIPE_ALL = 0x00,
    CREATE_VERTEX = 0x01,
    CREATE_TRIANGLE = 0x02,
    CREATE_INSTANCE = 0x03,
    UPDATE_INSTANCE = 0x04,

};

// Return codes:
enum {
    OK = 0x00,
    OUT_OF_MEMORY = 0x01,
    INVALID_ID = 0x02,
    INVALID_OPERATION = 0x03,
    INVALID_DATA = 0x04,
};

// SPI state machine states
typedef enum {
    SPI_IDLE,
    SPI_CMD_SENT,
    SPI_RX_WAIT,
    SPI_DONE,
    SPI_ERROR
} SPI_TransactionState;

volatile SPI_TransactionState spiState = SPI_IDLE;
volatile uint8_t spiReturnCode;
volatile uint8_t spiData;
volatile uint8_t txDone = 0;
volatile uint8_t rxDone = 0;

uint8_t rxBuffer[2]; // Buffer to hold received return code and data

// Function prototype for send_Buffer and receive_Buffer
static void send_Buffer(uint8_t *buf, uint32_t len);
static void receive_Buffer(uint8_t *buf, uint32_t len);

// DMA callbacks
void HAL_OSPI_TxCpltCallback(OSPI_HandleTypeDef *hospi) {
    txDone = 1;

    receive_Buffer(rxBuffer, 2); // Expecting 2 bytes: return code and optional data
    spiState = SPI_RX_WAIT;
}


void HAL_OSPI_RxCpltCallback(OSPI_HandleTypeDef *hospi) {
    rxDone = 1;
    spiReturnCode = rxBuffer[0] & 0x0F;   // Only lower nibble for return code
    spiData       = rxBuffer[1];           // Optional data
    spiState = SPI_DONE;
}

void HAL_OSPI_ErrorCallback(OSPI_HandleTypeDef *hospi) {
    spiState = SPI_ERROR;
}


// Op WIPE_ALL
// usage: wipe_all(); returns returncode sent by FPGA
uint8_t wipe_all(void) {
    if(spiState != SPI_IDLE) return 0xFF; // busy


    uint8_t cmd = WIPE_ALL;
    uint32_t len = 1;    // 1 byte command

    txDone = 0;
    rxDone = 0;
    spiState = SPI_CMD_SENT;

    send_Buffer(&cmd, len);

    return 0x00; // command sent successfully
}




// Op CREATE_VERTEX returns returnCode and one byte data containing the vertex ID
// vertexBuffer is a pointer to an array of vertices, each vertex is 3 floats (x, y, z) converted to 3 Q16.16
// uint16_t create_vertex(uint32_t *vertexBuffer, uint32_t vertCount) {
//     if(spiState != SPI_IDLE) return 0xFF;
//     if(vertCount == 0) return 0x01; // invalid data

//     uint8_t cmd = CREATE_VERTEX;
//     uint32_t len = vertCount * 3 * 4 + 1; // command byte + 3 floats per vertex, 4 bytes each
//     uint8_t buffer[len];
//     buffer[0] = cmd;
    

//     txDone = 0;
//     rxDone = 0;
//     spiState = SPI_CMD_SENT;
//     send_Buffer(&buffer, len);

// }

// Op CREATE_TRIANGLE returns returnCode and one byte data containing the triangle ID
// uint16_t create_triangle(uint32_t *triangleBuffer, uint32_t triCount) {

// }

// Op CREATE_INSTANCE returns returnCode and one byte data containing the instance ID
// uint16_t create_instance(uint8_t vertIDx, uint8_t triID, uint32_t *instanceBuffer) {

// }

// Op UPDATE_INSTANCE returns returnCode
// uint8_t update_instance(uint32_t *instanceBuffer, uint32_t instCount) {

// }

// Call periodically to check if SPI transaction is complete
uint8_t spi_service(uint8_t *data) {
    if(spiState == SPI_DONE) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET); // Deassert NCS
        spiState = SPI_IDLE;
        if(data) *data = spiData;
        return spiReturnCode;
    } else if(spiState == SPI_ERROR) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
        spiState = SPI_IDLE;
        return 0x15; // indicate SPI error
    }
    return 0xFF; // still busy
}

// Prepare default OSPI command structure, ospi_cmd is from main file
static void OSPI_ConfigRawWrite(uint32_t length)
{
    memset(&ospi_cmd, 0, sizeof(ospi_cmd));
    ospi_cmd.OperationType   = HAL_OSPI_OPTYPE_COMMON_CFG;
    ospi_cmd.FlashId         = HAL_OSPI_FLASH_ID_1;
    ospi_cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;    // not NONE
    ospi_cmd.Instruction     = 0x00;                          // dummy byte that FPGA should ignore
    ospi_cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    ospi_cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    ospi_cmd.AddressMode     = HAL_OSPI_ADDRESS_NONE;       // no address
    ospi_cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    ospi_cmd.DataMode        = HAL_OSPI_DATA_4_LINES;       // quad
    ospi_cmd.DummyCycles     = 0;
    ospi_cmd.NbData          = length;                      // number of bytes to transfer
    ospi_cmd.SIOOMode        = HAL_OSPI_SIOO_INST_EVERY_CMD;
}

//pin layout on nucleo board
//pin 0 -> CN10.24
//pin 1 -> CN7.34
//pin 2 -> CN10.15
//pin 3 -> CN10.13
//NCS -> CN7.28
static void send_Buffer(uint8_t *buf, uint32_t len)
{
    OSPI_ConfigRawWrite(len);

    if (HAL_OSPI_Command(&hospi1, &ospi_cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        spiState = SPI_ERROR;
        Error_Handler();
        return;
    }

    if (HAL_OSPI_Transmit_DMA(&hospi1, buf) != HAL_OK)
    {
        spiState = SPI_ERROR;
        Error_Handler();
        return;
    }
}

static void receive_Buffer(uint8_t *buf, uint32_t len)
{
    OSPI_ConfigRawWrite(len);

    if (HAL_OSPI_Command(&hospi1, &ospi_cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        spiState = SPI_ERROR;
        Error_Handler();
    }

    if (HAL_OSPI_Receive_DMA(&hospi1, buf) != HAL_OK)
    {
        spiState = SPI_ERROR;
        Error_Handler();
    }
}


#ifdef __cplusplus
}
#endif