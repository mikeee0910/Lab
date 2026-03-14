/**
  ******************************************************************************
  * @file    Wifi/WiFi_Client_Server/src/main.c
  * @author  MCD Application Team
  * @brief   This file provides main program functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32l475e_iot01_accelero.h"
#include "stm32l475e_iot01_gyro.h"
#include <stdio.h>
/* Private defines -----------------------------------------------------------*/

#define TERMINAL_USE

/* Update SSID and PASSWORD with own Access point settings */
//#define SSID     "ACLAB"
//#define PASSWORD "ACLAB3233"
#define SSID     "Meng"
#define PASSWORD "11112222"

uint8_t RemoteIP[] = {172,20,10,4};
#define RemotePORT	8002

#define WIFI_WRITE_TIMEOUT 10000
#define WIFI_READ_TIMEOUT  100

#define CONNECTION_TRIAL_MAX          10

#if defined (TERMINAL_USE)
#define TERMOUT(...)  printf(__VA_ARGS__)
#else
#define TERMOUT(...)
#endif

/* Private variables ---------------------------------------------------------*/
#if defined (TERMINAL_USE)
extern UART_HandleTypeDef hDiscoUart;
#endif /* TERMINAL_USE */
static uint8_t RxData [500];
volatile uint8_t Motion_Detected_Flag = 0 ;
extern void SENSOR_IO_Write(uint8_t Addr, uint8_t Reg, uint8_t Value);

/* Private function prototypes -----------------------------------------------*/
#if defined (TERMINAL_USE)
#ifdef __GNUC__
/* With GCC, small TERMOUT (option LD Linker->Libraries->Small TERMOUT
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
#endif /* TERMINAL_USE */

static void SystemClock_Config(void);
static void MX_GPIO_EXTI_Init(void);

extern  SPI_HandleTypeDef hspi;

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  uint8_t  MAC_Addr[6] = {0};
  uint8_t  IP_Addr[4] = {0};
  char TxData[100];        // 用來存放格式化後的感測器字串
  int16_t accel_data[3];   // 用來存放 X, Y, Z 三軸的加速度資料
  int32_t Socket = -1;
  uint16_t Datalen;
  int32_t ret;
  int16_t Trials = CONNECTION_TRIAL_MAX;

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();
  /* Configure LED2 */
  BSP_LED_Init(LED2);

#if defined (TERMINAL_USE)
  /* Initialize all configured peripherals */
  hDiscoUart.Instance = DISCOVERY_COM1;
  hDiscoUart.Init.BaudRate = 115200;
  hDiscoUart.Init.WordLength = UART_WORDLENGTH_8B;
  hDiscoUart.Init.StopBits = UART_STOPBITS_1;
  hDiscoUart.Init.Parity = UART_PARITY_NONE;
  hDiscoUart.Init.Mode = UART_MODE_TX_RX;
  hDiscoUart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hDiscoUart.Init.OverSampling = UART_OVERSAMPLING_16;
  hDiscoUart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hDiscoUart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  BSP_COM_Init(COM1, &hDiscoUart);
#endif /* TERMINAL_USE */

  TERMOUT("****** WIFI Module in TCP Client mode demonstration ****** \n\n");
  TERMOUT("TCP Client Instructions :\n");
  TERMOUT("1- Make sure your Phone is connected to the same network that\n");
  TERMOUT("   you configured using the Configuration Access Point.\n");
  TERMOUT("2- Create a server by using the android application TCP Server\n");
  TERMOUT("   with port(8002).\n");
  TERMOUT("3- Get the Network Name or IP Address of your Android from the step 2.\n\n");


  /* 初始化中斷接收腳位 */
    MX_GPIO_EXTI_Init();

    /* 初始化 3D 加速度計與顯著運動功能 */
    if (BSP_ACCELERO_Init() != ACCELERO_OK) {
      TERMOUT("> ERROR: Accelerometer Initialization Failed\n");
    } else {
      TERMOUT("> Accelerometer Initialized OK\n");


		// 改用 Wake-Up (任意晃動) 中斷功能
		// 1. 開啟基本中斷功能 (寫入 TAP_CFG 暫存器 0x58, 設為 0x80)
		SENSOR_IO_Write(0xD4, 0x58, 0x80);

		// 2. 設定晃動敏感度閾值 (寫入 WAKE_UP_THS 暫存器 0x5B, 設為 0x02，數字越小越敏感)
		SENSOR_IO_Write(0xD4, 0x5B, 0x02);

		// 3. 將 Wake-Up 中斷訊號綁定到 INT1 接腳 (寫入 MD1_CFG 暫存器 0x5E, 設為 0x20)
		SENSOR_IO_Write(0xD4, 0x5E, 0x20);
      TERMOUT("> Significant Motion Detection Enabled!\n");
    }

  /*Initialize  WIFI module */
  if(WIFI_Init() ==  WIFI_STATUS_OK)
  {
    TERMOUT("> WIFI Module Initialized.\n");
    if(WIFI_GetMAC_Address(MAC_Addr, sizeof(MAC_Addr)) == WIFI_STATUS_OK)
    {
      TERMOUT("> es-wifi module MAC Address : %X:%X:%X:%X:%X:%X\n",
               MAC_Addr[0],
               MAC_Addr[1],
               MAC_Addr[2],
               MAC_Addr[3],
               MAC_Addr[4],
               MAC_Addr[5]);
    }
    else
    {
      TERMOUT("> ERROR : CANNOT get MAC address\n");
      BSP_LED_On(LED2);
    }

    if( WIFI_Connect(SSID, PASSWORD, WIFI_ECN_WPA2_PSK) == WIFI_STATUS_OK)
    {
      TERMOUT("> es-wifi module connected \n");
      if(WIFI_GetIP_Address(IP_Addr, sizeof(IP_Addr)) == WIFI_STATUS_OK)
      {
        TERMOUT("> es-wifi module got IP Address : %d.%d.%d.%d\n",
               IP_Addr[0],
               IP_Addr[1],
               IP_Addr[2],
               IP_Addr[3]);

        TERMOUT("> Trying to connect to Server: %d.%d.%d.%d:%d ...\n",
               RemoteIP[0],
               RemoteIP[1],
               RemoteIP[2],
               RemoteIP[3],
							 RemotePORT);

        while (Trials--)
        {
          if( WIFI_OpenClientConnection(0, WIFI_TCP_PROTOCOL, "TCP_CLIENT", RemoteIP, RemotePORT, 0) == WIFI_STATUS_OK)
          {
            TERMOUT("> TCP Connection opened successfully.\n");
            Socket = 0;
            break;
          }
        }
        if(Socket == -1)
        {
          TERMOUT("> ERROR : Cannot open Connection\n");
          BSP_LED_On(LED2);
        }
      }
      else
      {
        TERMOUT("> ERROR : es-wifi module CANNOT get IP address\n");
        BSP_LED_On(LED2);
      }
    }
    else
    {
      TERMOUT("> ERROR : es-wifi module NOT connected\n");
      BSP_LED_On(LED2);
    }
  }
  else
  {
    TERMOUT("> ERROR : WIFI Module cannot be initialized.\n");
    BSP_LED_On(LED2);
  }

  while(1)
  {
      if(Socket != -1)
      {
          // ====== 1. 震動事件 ======
          if (Motion_Detected_Flag == 1)
          {
              char warningMsg[] = "\r\n[EVENT] Significant Motion Detected!\r\n";
              TERMOUT("[EVENT] Significant Motion Detected!\r\n"); // ← 改成字串字面量
              WIFI_SendData(Socket, (uint8_t*)warningMsg, strlen(warningMsg), &Datalen, WIFI_WRITE_TIMEOUT);
              Motion_Detected_Flag = 0;
              HAL_Delay(100); // ← 給 Wi-Fi 喘息，但不 continue
          }

          // ====== 2. 每輪都送 ACC（包含晃動當下的數值）======
          BSP_ACCELERO_AccGetXYZ(accel_data);
          sprintf(TxData, "ACC: X=%d, Y=%d, Z=%d\r\n", accel_data[0], accel_data[1], accel_data[2]);
          TERMOUT("Sending: %s", TxData);

          ret = WIFI_SendData(Socket, (uint8_t*)TxData, strlen(TxData), &Datalen, WIFI_WRITE_TIMEOUT);
          if (ret != WIFI_STATUS_OK)
          {
              TERMOUT("> ERROR : Failed to Send Data\n");
              break;
          }

          HAL_Delay(50);
      }
  }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (MSI)
  *            SYSCLK(Hz)                     = 80000000
  *            HCLK(Hz)                       = 80000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 1
  *            APB2 Prescaler                 = 1
  *            MSI Frequency(Hz)              = 4000000
  *            PLL_M                          = 1
  *            PLL_N                          = 40
  *            PLL_R                          = 2
  *            PLL_P                          = 7
  *            PLL_Q                          = 4
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* MSI is enabled after System reset, activate PLL with MSI as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLP = 7;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    while(1){
    	;
    }
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    /* Initialization Error */
    while(1);
  }
}

static void MX_GPIO_EXTI_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* 1. 開啟 GPIOD 的時鐘 (LSM6DSL 的 INT1 接在 PD11) */
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /* 2. 設定 PD11 為外部中斷輸入模式 (偵測上升緣 RISING) */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* 3. 設定中斷優先級並啟動 EXTI 線路 10 到 15 的中斷 */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}



#if defined (TERMINAL_USE)
/**
  * @brief  Retargets the C library TERMOUT function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART1 and Loop until the end of transmission */
  HAL_UART_Transmit(&hDiscoUart, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}
#endif /* TERMINAL_USE */

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: TERMOUT("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @brief  EXTI line detection callback.
  * @param  GPIO_Pin: Specifies the port pin connected to corresponding EXTI line.
  * @retval None
  */


void EXTI15_10_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
}

// ====== 把 PIN_11 加進去 ======
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch (GPIO_Pin)
  {
    case (GPIO_PIN_1): // 原本的 Wi-Fi 中斷
    {
      SPI_WIFI_ISR();
      break;
    }
    case (GPIO_PIN_11): // 新增的 LSM6DSL 顯著運動中斷
    {
      Motion_Detected_Flag = 1; // 舉起旗標，通知 while(1) 迴圈有震動發生
      break;
    }
    default:
    {
      break;
    }
  }
}
void SPI3_IRQHandler(void)
{
  HAL_SPI_IRQHandler(&hspi);
}
