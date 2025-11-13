/* USER CODE BEGIN Header */

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
#include "stm32u5xx_hal.h"
#include "tusb_input.hpp"
#include "usbh.h"

#ifdef SPI_TEST_MODE
extern "C" int spi_test_main(void);
#endif

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

OSPI_HandleTypeDef hospi1;
DMA_HandleTypeDef handle_GPDMA1_Channel0;
OSPI_RegularCmdTypeDef ospi_cmd;

SD_HandleTypeDef hsd_sdmmc1;

SPI_HandleTypeDef hspi2;

HCD_HandleTypeDef hhcd_USB_OTG_HS;

/* USER CODE BEGIN PV */
static FATFS sdFatFs;
static char SDPath[4];
static char gModelBasePath[64];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_GPDMA1_Init(void);
static void MX_OCTOSPI1_Init(void);
static void MX_USB_OTG_HS_HCD_Init(void);
static void MX_ADC1_Init(void);
static void MX_ICACHE_Init(void);
static void MX_SPI2_Init(void);
static void MX_SDMMC1_SD_Init(void);
/* USER CODE BEGIN PFP */
static void MX_GPDMA1_Init(void);
static HAL_StatusTypeDef Max7221_WriteRegister(uint8_t reg, uint8_t value);
static void Max7221_Init(void);
static void Max7221_DisplayNumber(uint32_t value);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
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
};

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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

static HAL_StatusTypeDef Max7221_WriteRegister(uint8_t reg, uint8_t value)
{
  uint8_t frame[2] = {reg, value};
  HAL_GPIO_WritePin(SPI2_NCS_GPIO_Port, SPI2_NCS_Pin, GPIO_PIN_RESET);
  const HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi2, frame, sizeof(frame), HAL_MAX_DELAY);
  HAL_GPIO_WritePin(SPI2_NCS_GPIO_Port, SPI2_NCS_Pin, GPIO_PIN_SET);

  if (status != HAL_OK)
  {
    printf("[MAX7221] SPI transmit failed: %d\r\n", status);
  }

  return status;
}

static void Max7221_Init(void)
{
  /* Configure while the driver is in shutdown to avoid spurious updates. */
  Max7221_WriteRegister(0x0C, 0x00U); /* Shutdown register -> shutdown mode */
  Max7221_WriteRegister(0x0F, 0x00U); /* Disable display test */
  Max7221_WriteRegister(0x09, 0xFFU); /* Code B decode for all digits */
  Max7221_WriteRegister(0x0B, 0x07U); /* Scan limit: display digits 0-7 */
  Max7221_WriteRegister(0x0A, 0x08U); /* Medium intensity */

  for (uint8_t digit = 1U; digit <= 8U; ++digit)
  {
    Max7221_WriteRegister(digit, 0x0FU); /* Blank all digits */
  }

  Max7221_WriteRegister(0x0C, 0x01U); /* Leave shutdown -> normal operation */
}

static void Max7221_DisplayNumber(uint32_t value)
{
  for (uint8_t digit = 1U; digit <= 8U; ++digit)
  {
    if ((value == 0U) && (digit > 1U))
    {
      Max7221_WriteRegister(digit, 0x0FU); /* Code B blank */
    }
    else
    {
      Max7221_WriteRegister(digit, static_cast<uint8_t>(value % 10U));
      value /= 10U;
    }
  }
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

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_OCTOSPI1_Init();
  MX_USB_OTG_HS_HCD_Init();
  MX_ADC1_Init();
  MX_ICACHE_Init();
  MX_SPI2_Init();
  // MX_SDMMC1_SD_Init(); //broken as per wednsday
  /* USER CODE BEGIN 2 */
  // automatic testing if enabled
  #ifdef SPI_TEST_MODE
  spi_test_main();
  #endif

  Max7221_Init();
  Max7221_DisplayNumber(0U);


  bool sdReady = false;
  bool runtimeMountOk = false;

  /* USER CODE END 2 */

  Rasterizer::SpiRasterizer rasterizer;
  // NullInput input;

  // Controller input
  tuh_init(0);

  HalTimer timer;
  Game game{rasterizer, TinyUSBInput::getInstance(), timer};

  if (sdReady)
  {
    runtimeMountOk = SD_MountForRuntime(gModelBasePath, sizeof(gModelBasePath));
    if (runtimeMountOk)
    {
      game.setModelBasePath(gModelBasePath);
    }
  }

  game.init();

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    game.tick_once();
    /* USER CODE END WHILE */
    tuh_task();
    TinyUSBInput::getInstance().driverTask();

    /* USER CODE BEGIN 3 */
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSI
                              |RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_MSI
                              |RCC_OSCILLATORTYPE_MSIK;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;
  RCC_OscInitStruct.MSIKClockRange = RCC_MSIKRANGE_5;
  RCC_OscInitStruct.MSIKState = RCC_MSIK_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV4;
  RCC_OscInitStruct.PLL.PLLM = 3;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = 5;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 1;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV16;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_14B;
  hadc1.Init.GainCompensation = 0;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_5CYCLE;
  sConfig.SingleDiff = ADC_DIFFERENTIAL_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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

  /** Enable instruction cache (default 2-ways set associative cache)
  */
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
  hospi1.Init.ClockPrescaler = 32;
  hospi1.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
  hospi1.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
  hospi1.Init.ChipSelectBoundary = 0;
  hospi1.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;

  /* USER CODE END OCTOSPI1_Init 0 */

  OSPIM_CfgTypeDef sOspiManagerCfg = {0};

  /* USER CODE BEGIN OCTOSPI1_Init 1 */

  /* USER CODE END OCTOSPI1_Init 1 */
  /* OCTOSPI1 parameter configuration*/

  if (HAL_OSPI_Init(&hospi1) != HAL_OK)
  {
    Error_Handler();
  }
  sOspiManagerCfg.ClkPort = 1;
  sOspiManagerCfg.NCSPort = 1;
  sOspiManagerCfg.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;
  if (HAL_OSPIM_Config(&hospi1, &sOspiManagerCfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN OCTOSPI1_Init 2 */

  /* USER CODE END OCTOSPI1_Init 2 */

}

/**
  * @brief SDMMC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDMMC1_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC1_Init 0 */

  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  /* USER CODE END SDMMC1_Init 1 */
  hsd_sdmmc1.Instance = SDMMC1;
  hsd_sdmmc1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd_sdmmc1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd_sdmmc1.Init.BusWide = SDMMC_BUS_WIDE_1B;
  hsd_sdmmc1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
  hsd_sdmmc1.Init.ClockDiv = kSdmmcTransferClockDiv;
  if (HAL_SD_Init(&hsd_sdmmc1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SDMMC1_Init 2 */
  /* USER CODE END SDMMC1_Init 2 */
}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  SPI_AutonomousModeConfTypeDef HAL_SPI_AutonomousMode_Cfg_Struct = {0};

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES_TXONLY;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 0x7;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  hspi2.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
  hspi2.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerState = SPI_AUTO_MODE_DISABLE;
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerSelection = SPI_GRP1_GPDMA_CH0_TCF_TRG;
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerPolarity = SPI_TRIG_POLARITY_RISING;
  if (HAL_SPIEx_SetConfigAutonomousMode(&hspi2, &HAL_SPI_AutonomousMode_Cfg_Struct) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief USB_OTG_HS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_HS_HCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_HS_Init 0 */

  /* USER CODE END USB_OTG_HS_Init 0 */

  /* USER CODE BEGIN USB_OTG_HS_Init 1 */

  /* USER CODE END USB_OTG_HS_Init 1 */
  hhcd_USB_OTG_HS.Instance = USB_OTG_HS;
  hhcd_USB_OTG_HS.Init.Host_channels = 16;
  hhcd_USB_OTG_HS.Init.speed = HCD_SPEED_HIGH;
  hhcd_USB_OTG_HS.Init.dma_enable = DISABLE;
  hhcd_USB_OTG_HS.Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
  hhcd_USB_OTG_HS.Init.Sof_enable = DISABLE;
  hhcd_USB_OTG_HS.Init.low_power_enable = DISABLE;
  hhcd_USB_OTG_HS.Init.use_external_vbus = ENABLE;
  if (HAL_HCD_Init(&hhcd_USB_OTG_HS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_HS_Init 2 */

  /* USER CODE END USB_OTG_HS_Init 2 */

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
  __HAL_RCC_LPGPIO1_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI2_NCS_GPIO_Port, SPI2_NCS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13
                          |GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure LPGPIO pins : Pin14 Pin15 Pin1 Pin2 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(LPGPIO1, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_NCS_Pin */
  GPIO_InitStruct.Pin = SPI2_NCS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI2_NCS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PD10 PD11 PD12 PD13
                           PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13
                          |GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PB6 PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
