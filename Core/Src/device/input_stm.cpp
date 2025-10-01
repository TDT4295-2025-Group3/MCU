// // SPI functions for communication with FPGA and implementation of MCU-FPGA protocol operations
// // MCU is configured as quad SPI master
// // The FPGA expects commands in a specific format
// // the first nibble is the OPCODE and the following varies by the operation being sent
// // the FPGA will respond with a single nibble return code followed by up to one byte of data
// // depending on the operation

// // constants for protocol commands
// // OPCODES 5 -> 15 is available for more commands
// // Return codes 5 -> 15 is available for more return codes

// #include <cstdint>
// #include "main.h"


// // Operations:
// enum {
//     WIPE_ALL = 0x00,
//     CREATE_VERTEX = 0x01,
//     CREATE_TRIANGLE = 0x02,
//     CREATE_INSTANCE = 0x03,
//     UPDATE_INSTANCE = 0x04,

// };

// // Return codes:
// enum {
//     OK = 0x00,
//     OUT_OF_MEMORY = 0x01,
//     INVALID_ID = 0x02,
//     INVALID_OPERATION = 0x03,
//     INVALID_DATA = 0x04,
// };

// // SPI state machine states
// typedef enum {
//     SPI_IDLE,
//     SPI_CMD_SENT,
//     SPI_RX_WAIT,
//     SPI_DONE,
//     SPI_ERROR
// } SPI_TransactionState;

// volatile SPI_TransactionState spiState = SPI_IDLE;
// volatile uint8_t spiReturnCode;
// volatile uint8_t spiData;
// volatile uint8_t txDone = 0;
// volatile uint8_t rxDone = 0;

// uint8_t rxBuffer[2]; // Buffer to hold received return code and data

// // Prepare OSPI command structure
// OSPI_RegularCmdTypeDef sCommand = {0};
// sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
// sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
// // We treat this as pure DATA phase — no “flash” instruction/address
// sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_NONE;
// sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
// sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
// // Data goes over 4 lines in quad mode
// sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
// sCommand.NbData             = sizeof(uint8_t);       // 1 needs to be configured per command
// sCommand.DummyCycles        = 0;
// sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
// sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;


// // DMA callbacks
// void HAL_OSPI_TxCpltCallback(OSPI_HandleTypeDef *hospi) {
//     txDone = 1;

//     sCommand.NbData = 2; // Expecting uop to 2 bytes back: return code + optional data
//     // Load the command into the OSPI peripheral
//     if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
//         printf("OSPI_Command failed!\n");
//         return 0x15;
//     }

//     // Start receiving 2 bytes: first = return code, second = optional data
//     if(HAL_OSPI_Receive_DMA(&hospi1, rxBuffer) != HAL_OK) {
//         HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
//         spiState = SPI_ERROR;
//     } else {
//         spiState = SPI_RX_WAIT;
//     }
// }


// void HAL_OSPI_RxCpltCallback(OSPI_HandleTypeDef *hospi) {
//     rxDone = 1;
//     spiReturnCode = rxBuffer[0] & 0x0F;   // Only lower nibble for return code
//     spiData       = rxBuffer[1];           // Optional data
//     spiState = SPI_DONE;
// }

// void HAL_OSPI_ErrorCallback(OSPI_HandleTypeDef *hospi) {
//     spiState = SPI_ERROR;    // Error occurred
// }


// // Op WIPE_ALL
// // usage: wipe_all(); returns returncode sent by FPGA
// extern "C" uint8_t wipe_all_start() {
//     if(spiState != SPI_IDLE) return 0xFF; // busy

//     uint8_t cmd = 0x0A;  // WIPE_ALL command

    
//     sCommand.NbData = 1; // Sending 1 byte command
//     // Load the command into the OSPI peripheral
//     if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
//         printf("OSPI_Command failed!\n");
//         return 0x15;
//     }


//     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);  // Assert NCS
//     txDone = 0;
//     rxDone = 0;
//     spiState = SPI_CMD_SENT;

//     if(HAL_OSPI_Transmit_DMA(&hospi1, &cmd) != HAL_OK) {
//         HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
//         spiState = SPI_ERROR;
//         return 0x15;  // error
//     }

//     return 0x00; // command sent successfully
// }

// extern "C" uint8_t spi_service(uint8_t *data) {
//     if(spiState == SPI_DONE) {
//         HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET); // Deassert NCS
//         spiState = SPI_IDLE;
//         if(data) *data = spiData;
//         return spiReturnCode;
//     } else if(spiState == SPI_ERROR) {
//         HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
//         spiState = SPI_IDLE;
//         return 0x15;
//     }
//     return 0xFF; // still busy
// }


// // Op CREATE_VERTEX returns returnCode and one byte data containing the vertex ID
// // vertexBuffer is a pointer to an array of vertices, each vertex is 3 floats (x, y, z) converted to 3 Q16.16
// // uint16_t create_vertex(uint32_t *vertexBuffer, uint32_t vertCount) {

// // }

// // Op CREATE_TRIANGLE returns returnCode and one byte data containing the triangle ID
// // uint16_t create_triangle(uint32_t *triangleBuffer, uint32_t triCount) {

// // }

// // Op CREATE_INSTANCE returns returnCode and one byte data containing the instance ID
// // uint16_t create_instance(uint8_t vertIDx, uint8_t triID, uint32_t *instanceBuffer) {

// // }

// // Op UPDATE_INSTANCE returns returnCode
// // uint8_t update_instance(uint32_t *instanceBuffer, uint32_t instCount) {

// // }


