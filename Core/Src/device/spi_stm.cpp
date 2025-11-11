#include "irasterizer.hpp"
// spi_stm_driver.cpp
#ifdef __cplusplus
extern "C" {
#endif

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>

#include "main.h"
#include "spi_stm.hpp"

#ifdef __cplusplus
}
#endif
// TODO: enable NVIC global interrupts for OCTOSPI and link callback functions correctly.

typedef enum {
    SPI_IDLE,
    SPI_CMD_SENT,
    SPI_RX_WAIT,
    SPI_DONE,
    SPI_ERROR
} SPI_TransactionState;

static volatile SPI_TransactionState spiState = SPI_IDLE;
static volatile uint8_t spiReturnCode = 0;
static volatile uint8_t spiData = 0;

static uint8_t rxBuffer[3] = {0}; // shared rx buffer for replies (return code + optional data)

// Callback and future/promise types
typedef void (*SpiTransferCallback)(uint8_t returnCode, uint8_t data, void* userCtx);



struct SpiJob {
    uint8_t* txBuf = nullptr;            // ownership: job owns this buffer and must free it
    uint32_t length = 0;
    SpiTransferCallback callback = nullptr;
    void* userCtx = nullptr;
    Rasterizer::SpiPromise* promise = nullptr;       // optional promise to fulfill
};

#define SPI_JOB_QUEUE_SIZE 8
static SpiJob spiJobQueue[SPI_JOB_QUEUE_SIZE];
static volatile uint8_t spiJobHead = 0;
static volatile uint8_t spiJobTail = 0;
static volatile uint8_t spiJobCount = 0;

// Forward declarations of helpers (defined below)
static void send_Buffer(uint8_t *buf, uint32_t len);
static void receive_Buffer(uint8_t *buf, uint32_t len);
static void OSPI_ConfigRawWrite(uint32_t length);
static void spiStartNextJob(void);

// Packet packing helper
static void pack_bits(uint8_t *buf, uint32_t *bit_offset, uint32_t value, uint8_t nbits) {
    for (int i = nbits - 1; i >= 0; i--) {
        uint32_t byte_pos = *bit_offset / 8;
        uint8_t bit_pos  = 7 - (*bit_offset % 8);
        buf[byte_pos] |= ((value >> i) & 1) << bit_pos;
        (*bit_offset)++;
    }
}

// forward declarations to avoid warnings
static void pack_create_vert_message(uint8_t *buffer, uint16_t numVerts, Rasterizer::Vertex *vertices);
static void pack_create_tri_message(uint8_t *buffer, uint16_t numTris, Rasterizer::Triangle *triangles);
static void pack_create_instance_message(uint8_t *buffer, uint8_t vertbuffID, uint8_t tribuffID, Rasterizer::Transform *instanceData);
static void pack_instance_update_message(uint8_t *buffer, Rasterizer::Transform *instanceData, uint8_t vertID, uint8_t triID, uint8_t instID);

// OSPI helper (uses global ospi_cmd and hospi1 from main project)
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

// send / receive wrappers
//pin layout on nucleo board
//pin 0 -> CN10.24
//pin 1 -> CN7.34
//pin 2 -> CN10.15
//pin 3 -> CN10.13
//NCS -> CN7.28
//CLK -> CN10.37
static void send_Buffer(uint8_t *buf, uint32_t len)
{
    OSPI_ConfigRawWrite(len+1); //+1 for dummy end byte to give FPGA more clock cycles

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
    OSPI_ConfigRawWrite(len+1);

    if (HAL_OSPI_Command(&hospi1, &ospi_cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        spiState = SPI_ERROR;
        Error_Handler();
        return;
    }

    if (HAL_OSPI_Receive_DMA(&hospi1, buf) != HAL_OK)
    {
        spiState = SPI_ERROR;
        Error_Handler();
        return;
    }
}

// Queue helpers
static bool spiQueueJob(uint8_t* txBuf, uint32_t len, SpiTransferCallback cb, void* ctx, Rasterizer::SpiPromise* promise)
{
    // Caller passes ownership of txBuf -> job takes ownership and will free it on completion
    if (spiJobCount >= SPI_JOB_QUEUE_SIZE) {
        return false; // queue full
    }

    // place job at tail
    SpiJob &job = spiJobQueue[spiJobTail];
    job.txBuf = txBuf;
    job.length = len;
    job.callback = cb;
    job.userCtx = ctx;
    job.promise = promise;

    spiJobTail = (spiJobTail + 1) % SPI_JOB_QUEUE_SIZE;
    spiJobCount= (spiJobCount + 1);

    // if idle, start next job now
    if (spiState == SPI_IDLE) {
        spiStartNextJob();
    }
    return true;
}

static void spiStartNextJob(void)
{
    if (spiJobCount == 0) {
        spiState = SPI_IDLE;
        return;
    }

    SpiJob &job = spiJobQueue[spiJobHead];
    spiState = SPI_CMD_SENT;

    // send job tx buffer using existing helper (it calls HAL_OSPI_Transmit_DMA)
    send_Buffer(job.txBuf, job.length);
}

// DMA HAL callbacks (C linkage via HAL)
// TX complete -> start RX (2 bytes expected)
extern "C" void HAL_OSPI_TxCpltCallback(OSPI_HandleTypeDef *hospi) {
    // start receive for reply (2 bytes): return nibble + optional 1-byte data
    // printf("TX complete, starting RX\r\n");
    receive_Buffer(rxBuffer, 3);
    spiState = SPI_RX_WAIT;
}

// RX complete -> read reply, call job callback/promise, free job buffer, advance queue
extern "C" void HAL_OSPI_RxCpltCallback(OSPI_HandleTypeDef *hospi) {
    // printf("RX complete, processing reply\r\n");
    spiReturnCode = ((rxBuffer[2] & 0xF0) >> 4) | (rxBuffer[2] & 0x0F);   // Only first nibble for return code
    spiData       = rxBuffer[1]; // Optional data in last nibble of first byte and first nibble of second byte
    spiState = SPI_DONE;

    // Safely capture the current job (head). ISR runs while main might enqueue, but head is updated here.
    if (spiJobCount == 0) {
        // spurious RX? just return to idle
        spiState = SPI_IDLE;
        return;
    }

    SpiJob &job = spiJobQueue[spiJobHead];

    // 1) If job supplied a callback, call it
    if (job.callback) {
        job.callback((uint8_t)spiReturnCode, (uint8_t)spiData, job.userCtx);
    }

    // 2) If job supplied a promise, fulfill it
    if (job.promise && job.promise->fut) {
        job.promise->fut->returnCode = (uint8_t)spiReturnCode;
        job.promise->fut->data = (uint8_t)spiData;
        job.promise->fut->done.store(true, std::memory_order_release);
        // free promise wrapper (it was allocated alongside future)
        delete job.promise;
        job.promise = nullptr;
    }

    // 3) free job tx buffer (owned by job)
    if (job.txBuf) {
        free(job.txBuf);
        job.txBuf = nullptr;
    }

    // 4) advance queue head
    spiJobHead = (spiJobHead + 1) % SPI_JOB_QUEUE_SIZE;
    spiJobCount = (spiJobCount - 1);

    // 5) start next job if any
    spiStartNextJob();
}

// Error callback: mark job as failed and notify via callback/promise
extern "C" void HAL_OSPI_ErrorCallback(OSPI_HandleTypeDef *hospi) {
    spiState = SPI_ERROR;

    // If there is a job in progress, notify it of an error
    if (spiJobCount > 0) {
        SpiJob &job = spiJobQueue[spiJobHead];

        const uint8_t errCode = static_cast<uint8_t>(Rasterizer::StatusCode::SPI_ERROR);
        const uint8_t errData = 0x00;

        if (job.callback) {
            job.callback(errCode, errData, job.userCtx);
        }
        if (job.promise && job.promise->fut) {
            job.promise->fut->returnCode = errCode;
            job.promise->fut->data = errData;
            job.promise->fut->done.store(true, std::memory_order_release);
            delete job.promise;
            job.promise = nullptr;
        }

        if (job.txBuf) {
            free(job.txBuf);
            job.txBuf = nullptr;
        }

        // advance queue
        spiJobHead = (spiJobHead + 1) % SPI_JOB_QUEUE_SIZE;
        spiJobCount = (spiJobCount - 1);
    }

    // try to recover by going idle; upper layer can reenqueue
    spiState = SPI_IDLE;
}

// Note: these functions allocate the packet buffer (malloc) and the returned SpiFuture
// Caller must delete the returned SpiFuture* when done.

static __attribute__((unused)) void spiFutureCallback(uint8_t code, uint8_t data, void* ctx) {
    // This callback is only used if caller prefers callback instead of promise.
    // We don't use it in future-based wrappers, but keep it for completeness.
    SpiPromise* p = (SpiPromise*)ctx;
    if (p && p->fut) {
        p->fut->returnCode = code;
        p->fut->data = data;
        p->fut->done.store(true, std::memory_order_release);
        delete p;
    }
}

// Helper to create a future and promise
static Rasterizer::SpiFuture* create_future_and_enqueue(uint8_t* buffer, uint32_t len)
{
    Rasterizer::SpiFuture* fut = new Rasterizer::SpiFuture();
    Rasterizer::SpiPromise* prom = new Rasterizer::SpiPromise();
    prom->fut = fut;

    bool ok = spiQueueJob(buffer, len, nullptr, nullptr, prom);
    if (!ok) {
        // queue full -> cleanup
        delete prom;
        delete fut;
        return nullptr;
    }
    return fut;
}

// Exported API functions (async futures)
extern "C" {

// wipe_all: simple command with no extra data
Rasterizer::SpiFuture* wipe_all_async(void) {
    uint8_t* buf = (uint8_t*)malloc(1);
    if (!buf) return nullptr;
    buf[0] = static_cast<uint8_t>(Rasterizer::Operation::WIPE_ALL) << 4;
    return create_future_and_enqueue(buf, 1);
}

// create_vertex_async: allocate and pack, return a future
Rasterizer::SpiFuture* create_vertex_async(Rasterizer::Vertex *vertexBuffer, uint16_t vertCount) {
    if (vertCount == 0) return nullptr;


    // compute length similarly to your previous formula
    uint32_t len = 2 + ((vertCount * 108 + 7) / 8);
    uint8_t* buffer = (uint8_t*)malloc(len);
    if (!buffer) return nullptr;

    // pack message (the function will convert floats to Q16.16 in-place)
    pack_create_vert_message(buffer, vertCount, vertexBuffer);

    Rasterizer::SpiFuture* fut = create_future_and_enqueue(buffer, len);
    if (!fut) {
        free(buffer);
    }
    return fut;
}

Rasterizer::SpiFuture* create_triangle_async(Rasterizer::Triangle *triangleBuffer, uint16_t triCount) {
    if (triCount == 0) return nullptr;
    uint32_t len = 2 + ((triCount * 36 + 7) / 8);
    uint8_t* buffer = (uint8_t*)malloc(len);
    if (!buffer) return nullptr;

    pack_create_tri_message(buffer, triCount, triangleBuffer);

    Rasterizer::SpiFuture* fut = create_future_and_enqueue(buffer, len);
    if (!fut) free(buffer);
    return fut;
}

Rasterizer::SpiFuture* create_instance_async(Rasterizer::Transform *instanceData, uint8_t vertbufferID, uint8_t tribufferID) {
    uint32_t len = 3 + 12*4;
    uint8_t* buffer = (uint8_t*)malloc(len);
    if (!buffer) return nullptr;

    pack_create_instance_message(buffer, vertbufferID, tribufferID, instanceData);

    Rasterizer::SpiFuture* fut = create_future_and_enqueue(buffer, len);
    if (!fut) free(buffer);
    return fut;
}

Rasterizer::SpiFuture* update_instance_async(Rasterizer::Transform *instanceData, uint8_t vertID, uint8_t triID, uint8_t instanceId) {
    uint32_t len = 4 + 12*4;
    uint8_t* buffer = (uint8_t*)malloc(len);
    if (!buffer) return nullptr;

    pack_instance_update_message(buffer, instanceData, vertID, triID, instanceId);

    Rasterizer::SpiFuture* fut = create_future_and_enqueue(buffer, len);
    if (!fut) free(buffer);
    return fut;
}

// spi_service: optional polling-style API for code that doesn't want futures.
// If you pass a pointer to data, it will be populated and the function will return the return code.
// Returns: 0xFF = busy, 0x15 = SPI error, otherwise return code.
__attribute__((unused)) uint8_t  spi_service_poll(uint8_t *data) {
    if (spiState == SPI_DONE) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET); // Deassert NCS
        spiState = SPI_IDLE;
        if (data) *data = spiData;
        return spiReturnCode;
    } else if (spiState == SPI_ERROR) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
        spiState = SPI_IDLE;
        return 0x15; // indicate SPI error
    }
    return 0xFF; // still busy
}

} // extern "C"

// Pack functions
static void pack_create_vert_message(uint8_t *buffer, uint16_t numVerts, Rasterizer::Vertex *vertices) {
    uint32_t bufsize = 2 + ((numVerts * 108 + 7) / 8);
    memset(buffer, 0, bufsize);

    // 1) Pack header: 4-bit opcode + 12-bit vertex count
    uint16_t header = (static_cast<uint8_t>(Rasterizer::Operation::CREATE_VERT) << 12) | (numVerts & 0x0FFF);
    buffer[0] = header >> 8;
    buffer[1] = header & 0xFF;

    uint32_t bit_offset = 16; // start after header

    // 2) Pack vertices (convert floats -> Q16.16 and pack)
    for (uint16_t i = 0; i < numVerts; i++) {
        Rasterizer::Vertex v = vertices[i]; // local copy so we don't overwrite caller memory

        int32_t qx = floatToQ16_16(v.x);
        int32_t qy = floatToQ16_16(v.y);
        int32_t qz = floatToQ16_16(v.z);

        pack_bits(buffer, &bit_offset, (uint32_t)qx, 32);
        pack_bits(buffer, &bit_offset, (uint32_t)qy, 32);
        pack_bits(buffer, &bit_offset, (uint32_t)qz, 32);

        // pack r,g,b (4 bits each)
        pack_bits(buffer, &bit_offset, v.r & 0x0F, 4);
        pack_bits(buffer, &bit_offset, v.g & 0x0F, 4);
        pack_bits(buffer, &bit_offset, v.b & 0x0F, 4);
    }
}

static void pack_create_tri_message(uint8_t *buffer, uint16_t numTris, Rasterizer::Triangle *triangles) {
    uint32_t bufsize = 2 + ((numTris * 36 + 7) / 8);
    memset(buffer, 0, bufsize);

    uint16_t header = (static_cast<uint8_t>(Rasterizer::Operation::CREATE_TRI) << 12) | (numTris & 0x0FFF);
    buffer[0] = header >> 8;
    buffer[1] = header & 0xFF;

    uint32_t bit_offset = 16;
    for (uint16_t i = 0; i < numTris; i++) {
        Rasterizer::Triangle t = triangles[i];
        pack_bits(buffer, &bit_offset, t.index0 & 0x0FFF, 12);
        pack_bits(buffer, &bit_offset, t.index1 & 0x0FFF, 12);
        pack_bits(buffer, &bit_offset, t.index2 & 0x0FFF, 12);
    }
}

static void pack_create_instance_message(uint8_t *buffer, uint8_t vertbuffID, uint8_t tribuffID, Rasterizer::Transform *instanceData) {
    uint32_t bufsize = 3 + 12*4;
    memset(buffer, 0, bufsize);
    uint32_t cmd = static_cast<uint8_t>(Rasterizer::Operation::CREATE_INST);
    uint32_t bit_offset = 0;


    pack_bits(buffer, &bit_offset, cmd, 4);
    pack_bits(buffer, &bit_offset, vertbuffID, 8);
    pack_bits(buffer, &bit_offset, tribuffID, 8);

    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(instanceData->posX), 32);
    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(instanceData->posY), 32);
    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(instanceData->posZ), 32);

    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(sin(instanceData->rotX)), 32);
    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(sin(instanceData->rotY)), 32);
    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(sin(instanceData->rotZ)), 32);

    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(cos(instanceData->rotX)), 32);
    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(cos(instanceData->rotY)), 32);
    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(cos(instanceData->rotZ)), 32);

    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(instanceData->scaleX), 32);
    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(instanceData->scaleY), 32);
    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(instanceData->scaleZ), 32);
}

static void pack_instance_update_message(uint8_t *buffer, Rasterizer::Transform *instanceData, uint8_t vertID, uint8_t triID, uint8_t instID) {
    uint32_t bufsize = 4 + 12*4;
    memset(buffer, 0, bufsize);
    uint32_t cmd = static_cast<uint8_t>(Rasterizer::Operation::UPDATE_INST);
    uint32_t bit_offset = 0;


    pack_bits(buffer, &bit_offset, cmd, 4);
    pack_bits(buffer, &bit_offset, vertID, 8);
    pack_bits(buffer, &bit_offset, triID, 8);
    pack_bits(buffer, &bit_offset, instID, 8);

    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(instanceData->posX), 32);
    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(instanceData->posY), 32);
    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(instanceData->posZ), 32);

    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(sin(instanceData->rotX)), 32);
    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(sin(instanceData->rotY)), 32);
    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(sin(instanceData->rotZ)), 32);

    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(cos(instanceData->rotX)), 32);
    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(cos(instanceData->rotY)), 32);
    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(cos(instanceData->rotZ)), 32);

    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(instanceData->scaleX), 32);
    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(instanceData->scaleY), 32);
    pack_bits(buffer, &bit_offset, (uint32_t)floatToQ16_16(instanceData->scaleZ), 32);
}


namespace Rasterizer {
    Rasterizer::SpiFuture* SpiAsyncRasterizer::wipeAllAsync() {
        return wipe_all_async();
    }

    Rasterizer::SpiFuture* SpiAsyncRasterizer::createVertexAsync(const Vertex* vertices, uint16_t count) {
        // cast away const since the SPI layer takes non-const (we can fix that later)
        return create_vertex_async(const_cast<Vertex*>(vertices), count);
    }

    Rasterizer::SpiFuture* SpiAsyncRasterizer::createTriangleAsync(const Triangle* triangles, uint16_t count) {
        return create_triangle_async(const_cast<Triangle*>(triangles), count);
    }

    Rasterizer::SpiFuture* SpiAsyncRasterizer::createInstanceAsync(uint8_t vertexId, uint8_t triangleId, const Transform& transform) {
        return create_instance_async(const_cast<Transform*>(&transform), vertexId, triangleId);
    }

    Rasterizer::SpiFuture* SpiAsyncRasterizer::updateInstanceAsync(uint8_t vertID, uint8_t triID, uint8_t instanceId, const Transform& transform) {
        return update_instance_async(const_cast<Transform*>(&transform), vertID, triID, instanceId);
    }
}