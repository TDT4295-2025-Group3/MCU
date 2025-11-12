/*********************************************************************************************
*                                                                                           
*   @@@@@@@@@@@@@@@@@@@@@@@@@@@                                                             
*  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@                                                            
*  @@@                       @@@                                                            
*  @@@                       @@@                                                            
*  @@@     @@         @@     @@@                                                            
*  @@@   @@@@@@     @@@@@@   @@@                                                            
*  @@@    @@@@       @@@@    @@@                                                            
*  @@@    @@@@       @@@@    @@@                                                            
*  @@@    @@@@       @@@@    @@@                                                            
*  @@@    @@@@       @@@@    @@@                                                     @@@@   
*  @@@    @@@@       @@@@    @@@                                                     @@@@   
*  @@@    @@@@       @@@@                                                            @@@@   
*  @@@    @@@@       @@@@    @@@@@@@@@   @@@   @@@   @@@   @@@@@@@@   @@@@@@@  @@@@@@@@@@   
*  @@@    @@@@       @@@@    @@@    @@@   @@@  @@@@  @@@        @@@   @@@     @@@@   @@@@   
*  @@@    @@@@       @@@@    @@@    @@@@  @@@ @@ @@ @@@    @@@@@@@@   @@@     @@@    @@@@   
*  @@@    @@@@@     @@@@@    @@@    @@@    @@@@@ @@@@@@   @@@   @@@   @@@     @@@@   @@@@   
*  @@@     @@@@@@@@@@@@@     @@@@@@@@@@    @@@@   @@@@    @@@@  @@@   @@@      @@@@@@@@@@   
*  @@@        @@@@@@@        @@@@@@@@       @@@   @@@      @@@@@@@@   @@@        @@@@@@@@   
*  @@@                       @@@                                                            
*  @@@ @@@@@@@@@@@@@@@@@@@@@ @@@                                                            
*  @@@                                                                                      
*  @@@                       @@@                                                            
*  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@                                                            
*    @@@@@@@@@@@@@@@@@@@@@@@@@                                                              
*
*********************************************************************************************/

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.cpp
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "ff.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"

#include <cstdio>
#include <cstring>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <cstdio>
#include "spi_stm.hpp"
#include "app/game.hpp"
#include "app/platform/iinput.hpp"
#include "app/platform/itimer.hpp"
#include "app/platform/irasterizer.hpp"

#ifdef SPI_TEST_MODE
extern "C" int spi_test_main(void);
#endif

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
uint8_t txBuffer[256];   // test buffer

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;

OSPI_HandleTypeDef hospi1;
DMA_HandleTypeDef handle_GPDMA1_Channel0;
SD_HandleTypeDef hsd_sdmmc1;

static FATFS sdFatFs;
static char SDPath[4];
static char gModelBasePath[64];

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void SystemPower_Config(void);
static void MX_GPIO_Init(void);
static void MX_GPDMA1_Init(void);
static void MX_ICACHE_Init(void);
static void MX_OCTOSPI1_Init(void);
static bool MX_SDMMC1_SD_Init(void);
/* USER CODE BEGIN PFP */
extern "C" int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}
namespace
{
constexpr uint32_t kSdmmcTransferClockDiv = 8U; // 48 MHz / (2 * 8) ~= 3 MHz
constexpr char kModelDirectory[] = "models";

class NullInput : public IInput
{
public:
  KeyState poll() override { return {}; }
};

class HalTimer : public ITimer
{
public:
  uint32_t get_ticks_ms() override { return HAL_GetTick(); }
};

class NullRasterizer : public Rasterizer::IRasterizer
{
public:
  void clear(uint32_t) override {}
  void rect(uint16_t, uint16_t, uint16_t, uint16_t, uint32_t) override {}
  void end_frame() override {}

  Rasterizer::WipeAllResponse wipeAll() override
  {
    return Rasterizer::WipeAllResponse(Rasterizer::StatusCode::OK);
  }

  Rasterizer::CreateVertResponse createVertex(const Rasterizer::Vertex*, uint16_t) override
  {
    const uint8_t id = nextVertexId++;
    return Rasterizer::CreateVertResponse(Rasterizer::StatusCode::OK, id);
  }

  Rasterizer::CreateTriResponse createTriangle(const Rasterizer::Triangle*, uint16_t) override
  {
    const uint8_t id = nextTriangleId++;
    return Rasterizer::CreateTriResponse(Rasterizer::StatusCode::OK, id);
  }

  Rasterizer::CreateInstResponse createInstance(uint8_t, uint8_t, const Rasterizer::Transform&) override
  {
    const uint8_t id = nextInstanceId++;
    return Rasterizer::CreateInstResponse(Rasterizer::StatusCode::OK, id);
  }

  Rasterizer::UpdateInstResponse updateInstance(uint8_t vertID, uint8_t triID, uint8_t instanceId, const Rasterizer::Transform&) override
  {
    static_cast<void>(vertID);
    static_cast<void>(triID);
    static_cast<void>(instanceId);
    return Rasterizer::UpdateInstResponse(Rasterizer::StatusCode::OK);
  }

  Rasterizer::UpdateInstResponse updateCamera(const Rasterizer::Transform&)
  {
    return Rasterizer::UpdateInstResponse(Rasterizer::StatusCode::OK);
  }

  Rasterizer::SpiFuture* wipeAllAsync(Rasterizer::FutureCallback, void*) override { return nullptr; }
  Rasterizer::SpiFuture* createVertexAsync(const Rasterizer::Vertex*, uint16_t,
                                           Rasterizer::FutureCallback, void*) override
  {
    return nullptr;
  }
  Rasterizer::SpiFuture* createTriangleAsync(const Rasterizer::Triangle*, uint16_t,
                                             Rasterizer::FutureCallback, void*) override
  {
    return nullptr;
  }
  Rasterizer::SpiFuture* createInstanceAsync(uint8_t, uint8_t, const Rasterizer::Transform&,
                                             Rasterizer::FutureCallback, void*) override
  {
    return nullptr;
  }
  Rasterizer::SpiFuture* updateInstanceAsync(uint8_t, uint8_t, uint8_t, const Rasterizer::Transform&,
                                             Rasterizer::FutureCallback, void*) override
  {
    return nullptr;
  }

private:
  uint8_t nextVertexId = 0;
  uint8_t nextTriangleId = 0;
  uint8_t nextInstanceId = 0;
};
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static bool SD_CardSelfTest(void)
{
  if (FATFS_LinkDriver(&SD_Driver, SDPath) != 0)
  {
    printf("[SD] FATFS_LinkDriver failed\r\n");
    return false;
  }

  bool mounted = false;
  bool fileOpen = false;
  bool fileCreated = false;
  bool success = false;

  FRESULT res = FR_OK;
  FIL file{};
  char filePath[32] = {0};
  HAL_SD_CardInfoTypeDef cardInfo{};
  constexpr char testData[] = "FatFs SD card test OK\r\n";
  char readBuffer[sizeof(testData)] = {0};
  UINT bytesWritten = 0;
  UINT bytesRead = 0;

  do
  {
    const HAL_SD_CardStateTypeDef cardStateBefore = HAL_SD_GetCardState(&hsd_sdmmc1);
    printf("[SD] Card state before mount: %d\r\n", static_cast<int>(cardStateBefore));

    HAL_SD_CardInfoTypeDef preMountInfo{};
    if (HAL_SD_GetCardInfo(&hsd_sdmmc1, &preMountInfo) == HAL_OK)
    {
      printf("[SD] Info before mount: RCA=0x%04lx, blocks=%lu, blockSize=%lu\r\n",
             static_cast<unsigned long>(preMountInfo.RelCardAdd),
             static_cast<unsigned long>(preMountInfo.LogBlockNbr),
             static_cast<unsigned long>(preMountInfo.LogBlockSize));
    }
    else
    {
      printf("[SD] HAL_SD_GetCardInfo pre-mount failed, err=0x%08lx\r\n",
             HAL_SD_GetError(&hsd_sdmmc1));
    }

    res = f_mount(&sdFatFs, SDPath, 1);
    if (res != FR_OK)
    {
      printf("[SD] f_mount failed: %d\r\n", res);
      printf("[SD] Card state after failed mount: %d (error=0x%08lx)\r\n",
             static_cast<int>(HAL_SD_GetCardState(&hsd_sdmmc1)),
             HAL_SD_GetError(&hsd_sdmmc1));
      const DSTATUS diskStat = SD_Driver.disk_status(0);
      printf("[SD] Disk status flags: 0x%02x\r\n", diskStat);
      break;
    }
    mounted = true;

    if (HAL_SD_GetCardInfo(&hsd_sdmmc1, &cardInfo) == HAL_OK)
    {
      const uint32_t blockSize = cardInfo.LogBlockSize;
      const uint64_t capacityBytes = static_cast<uint64_t>(cardInfo.LogBlockNbr) * blockSize;
      const unsigned long capacityMB = static_cast<unsigned long>(capacityBytes / (1024ULL * 1024ULL));
      printf("[SD] Card detected: %lu blocks x %lu bytes (%lu MB)\r\n",
             static_cast<unsigned long>(cardInfo.LogBlockNbr),
             static_cast<unsigned long>(blockSize),
             capacityMB);
    }
    else
    {
      printf("[SD] HAL_SD_GetCardInfo failed, err=0x%08lx\r\n",
             HAL_SD_GetError(&hsd_sdmmc1));
    }

    std::snprintf(filePath, sizeof(filePath), "%stest.txt", SDPath);

    res = f_open(&file, filePath, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
      printf("[SD] f_open(write) failed: %d\r\n", res);
      break;
    }
    fileOpen = true;
    fileCreated = true;

    res = f_write(&file, testData, sizeof(testData) - 1, &bytesWritten);
    if (res != FR_OK || bytesWritten != sizeof(testData) - 1)
    {
      printf("[SD] f_write failed: %d (bytes=%u)\r\n", res, bytesWritten);
      break;
    }

    res = f_close(&file);
    if (res != FR_OK)
    {
      printf("[SD] f_close(write) failed: %d\r\n", res);
      break;
    }
    fileOpen = false;

    res = f_open(&file, filePath, FA_READ);
    if (res != FR_OK)
    {
      printf("[SD] f_open(read) failed: %d\r\n", res);
      break;
    }
    fileOpen = true;

    res = f_read(&file, readBuffer, sizeof(testData) - 1, &bytesRead);
    if (res != FR_OK)
    {
      printf("[SD] f_read failed: %d\r\n", res);
      break;
    }

    if (bytesRead != sizeof(testData) - 1 || std::memcmp(readBuffer, testData, sizeof(testData) - 1) != 0)
    {
      printf("[SD] Data mismatch (read %u bytes)\r\n", bytesRead);
      break;
    }

    printf("[SD] Read back: %s", readBuffer);
    success = true;
  }
  while (false);

  if (fileOpen)
  {
    f_close(&file);
  }

  if (mounted)
  {
    if (fileCreated)
    {
      f_unlink(filePath);
    }
    f_mount(nullptr, SDPath, 0);
  }

  FATFS_UnLinkDriver(SDPath);

  printf("[SD] Self-test %s\r\n", success ? "PASSED" : "FAILED");

  return success;
}

static bool SD_MountForRuntime(char* modelBasePath, size_t maxLen)
{
  if (FATFS_LinkDriver(&SD_Driver, SDPath) != 0)
  {
    printf("[SD] FATFS_LinkDriver runtime failed\r\n");
    return false;
  }

  const FRESULT mountRes = f_mount(&sdFatFs, SDPath, 1);
  if (mountRes != FR_OK)
  {
    printf("[SD] f_mount runtime failed: %d\r\n", mountRes);
    FATFS_UnLinkDriver(SDPath);
    return false;
  }

  char dirPath[sizeof(gModelBasePath)] = {0};
  const int dirWritten = std::snprintf(dirPath, sizeof(dirPath), "%s%s", SDPath, kModelDirectory);
  if ((dirWritten <= 0) || (dirWritten >= static_cast<int>(sizeof(dirPath))))
  {
    printf("[SD] models directory path too long\r\n");
    f_mount(nullptr, SDPath, 0);
    FATFS_UnLinkDriver(SDPath);
    return false;
  }

  const FRESULT dirRes = f_mkdir(dirPath);
  if (dirRes != FR_OK && dirRes != FR_EXIST)
  {
    printf("[SD] f_mkdir('%s') failed: %d\r\n", dirPath, dirRes);
    f_mount(nullptr, SDPath, 0);
    FATFS_UnLinkDriver(SDPath);
    return false;
  }

  const int baseWritten = std::snprintf(modelBasePath, maxLen, "%s%s/", SDPath, kModelDirectory);
  if ((baseWritten <= 0) || (baseWritten >= static_cast<int>(maxLen)))
  {
    printf("[SD] Model base path buffer too small\r\n");
    f_mount(nullptr, SDPath, 0);
    FATFS_UnLinkDriver(SDPath);
    return false;
  }

  printf("[SD] Runtime mount OK (base=%s)\r\n", modelBasePath);
  return true;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  /* USER CODE END Init */

  /* Configure the System Power */
  SystemPower_Config();

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_ICACHE_Init();
  MX_OCTOSPI1_Init();
  /* USER CODE BEGIN 2 */

  // automatic testing if enabled
  #ifdef SPI_TEST_MODE
  spi_test_main();
  #endif
  bool sdReady = false;
  bool runtimeMountOk = false;
  uint32_t lastBlinkMs = HAL_GetTick();
  bool errorBlinkShortPhase = false;

  /* USER CODE END 2 */

  /* Initialize led */
  BSP_LED_Init(LED_GREEN);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);



  BSP_COM_SelectLogPort(COM1);

  setvbuf(stdout, nullptr, _IONBF, 0);
  printf("Console ready\r\n");



  const bool sdInitOk = MX_SDMMC1_SD_Init();
  if (sdInitOk)
  {
    sdReady = SD_CardSelfTest();

    if (sdReady)
    {
      printf("[SD] Card communication OK\r\n");
    }
    else
    {
      printf("[SD] Card communication FAILED\r\n");
    }
  }
  else
  {
    printf("[SD] Controller init failed\r\n");
  }

  NullInput input;
  HalTimer timer;
  NullRasterizer nullRasterizer;
  Game game{nullRasterizer, input, timer};

  if (sdReady)
  {
    runtimeMountOk = SD_MountForRuntime(gModelBasePath, sizeof(gModelBasePath));
    if (runtimeMountOk)
    {
      game.setModelBasePath(gModelBasePath);
    }
  }

  game.init();
  const bool sdOperational = sdReady && runtimeMountOk;

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {


    // wipe_all_start();


    /* USER CODE BEGIN 2 */


    /* USER CODE END 2 */



    /* USER CODE END WHILE */
    game.tick_once();

    const uint32_t now = HAL_GetTick();
    if (sdOperational)
    {
      if ((now - lastBlinkMs) >= 200U)
      {
        HAL_GPIO_TogglePin(LED2_GPIO_PORT, LED2_PIN);
        lastBlinkMs = now;
      }
    }
    else
    {
      const uint32_t waitMs = errorBlinkShortPhase ? 150U : 1000U;
      if ((now - lastBlinkMs) >= waitMs)
      {
        HAL_GPIO_TogglePin(LED2_GPIO_PORT, LED2_PIN);
        lastBlinkMs = now;
        errorBlinkShortPhase = !errorBlinkShortPhase;
      }
    }

    HAL_Delay(1);
    /* USER CODE BEGIN 3 */
    HAL_Delay(10);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE4) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI|RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Power Configuration
  * @retval None
  */
static void SystemPower_Config(void)
{

  /*
   * Switch to SMPS regulator instead of LDO
   */
  if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK)
  {
    Error_Handler();
  }
/* USER CODE BEGIN PWR */
/* USER CODE END PWR */
}

/**
  * @brief GPDMA1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPDMA1_Init(void)
{

  /* USER CODE BEGIN GPDMA1_Init 0 */

  /* USER CODE END GPDMA1_Init 0 */

  /* Peripheral clock enable */
  __HAL_RCC_GPDMA1_CLK_ENABLE();

  /* GPDMA1 interrupt Init */
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

  /* USER CODE BEGIN GPDMA1_Init 1 */

  /* USER CODE END GPDMA1_Init 1 */
  /* USER CODE BEGIN GPDMA1_Init 2 */

  /* USER CODE END GPDMA1_Init 2 */

}

/**
  * @brief ICACHE Initialization Function
  * @param None
  * @retval None
  */
static void MX_ICACHE_Init(void)
{

  /* USER CODE BEGIN ICACHE_Init 0 */

  /* USER CODE END ICACHE_Init 0 */

  /* USER CODE BEGIN ICACHE_Init 1 */

  /* USER CODE END ICACHE_Init 1 */

  /** Enable instruction cache in 1-way (direct mapped cache)
  */
  if (HAL_ICACHE_ConfigAssociativityMode(ICACHE_1WAY) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_ICACHE_Enable() != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ICACHE_Init 2 */

  /* USER CODE END ICACHE_Init 2 */

}

/**
  * @brief OCTOSPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_OCTOSPI1_Init(void)
{

  /* USER CODE BEGIN OCTOSPI1_Init 0 */
  hospi1.Instance = OCTOSPI1;
  hospi1.Init.FifoThreshold = 4;
  hospi1.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;
  hospi1.Init.DeviceSize = 24;
  hospi1.Init.ChipSelectHighTime = 1;
  hospi1.Init.ClockPrescaler = 16;
  hospi1.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
  hospi1.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
  hospi1.Init.ChipSelectBoundary = 0;
  hospi1.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
  /* USER CODE END OCTOSPI1_Init 0 */

  /* USER CODE BEGIN OCTOSPI1_Init 1 */

  /* USER CODE END OCTOSPI1_Init 1 */
  /* OCTOSPI1 parameter configuration*/
  
  if (HAL_OSPI_Init(&hospi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN OCTOSPI1_Init 2 */

  /* USER CODE END OCTOSPI1_Init 2 */

}

OSPI_RegularCmdTypeDef ospi_cmd;


// static void OSPI_ConfigRawWrite(uint32_t length)
// {
//     memset(&ospi_cmd, 0, sizeof(ospi_cmd));
//     ospi_cmd.OperationType   = HAL_OSPI_OPTYPE_COMMON_CFG;
//     ospi_cmd.FlashId         = HAL_OSPI_FLASH_ID_1;
//     ospi_cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;    // not NONE
//     ospi_cmd.Instruction     = 0x00;                          // dummy byte that FPGA should ignore
//     ospi_cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
//     ospi_cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
//     ospi_cmd.AddressMode     = HAL_OSPI_ADDRESS_NONE;       // no address
//     ospi_cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
//     ospi_cmd.DataMode        = HAL_OSPI_DATA_4_LINES;       // quad
//     ospi_cmd.DummyCycles     = 0;
//     ospi_cmd.NbData          = length;                      // number of bytes to transfer
//     ospi_cmd.SIOOMode        = HAL_OSPI_SIOO_INST_EVERY_CMD;
// }



//pin 0 -> CN10.24
//pin 1 -> CN7.34
//pin 2 -> CN10.15
//pin 3 -> CN10.13
// static void Send_Buffer(uint8_t *buf, uint32_t len)
// {
//     OSPI_ConfigRawWrite(len);

//     if (HAL_OSPI_Command(&hospi1, &ospi_cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
//     {
//         Error_Handler();
//     }

//     if (HAL_OSPI_Transmit_DMA(&hospi1, buf) != HAL_OK)
//     {
//         Error_Handler();
//     }
// }

// void HAL_OSPI_TxCpltCallback(OSPI_HandleTypeDef *hospi)
// {
//     txDone = true;
// }


static bool MX_SDMMC1_SD_Init(void)
{
  hsd_sdmmc1.Instance = SDMMC1;
  hsd_sdmmc1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd_sdmmc1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd_sdmmc1.Init.BusWide = SDMMC_BUS_WIDE_1B;
  hsd_sdmmc1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
  hsd_sdmmc1.Init.ClockDiv = kSdmmcTransferClockDiv;
  if (HAL_SD_Init(&hsd_sdmmc1) != HAL_OK)
  {
    printf("[SD] HAL_SD_Init error: 0x%08lx\r\n", HAL_SD_GetError(&hsd_sdmmc1));
    return false;
  }

  const uint32_t sdmmcClkHz = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SDMMC);
  uint32_t transferClkHz = 0U;
  if (sdmmcClkHz != 0U)
  {
    transferClkHz = (kSdmmcTransferClockDiv == 0U)
                      ? sdmmcClkHz
                      : (sdmmcClkHz / (2U * kSdmmcTransferClockDiv));
  }
  printf("[SD] Transfer clock configured ~ %lu Hz\r\n", static_cast<unsigned long>(transferClkHz));

  return true;
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin : PC10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
