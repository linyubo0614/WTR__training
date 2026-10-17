/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include <stdint.h>
#include <stdbool.h>


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
#define RX_RING_SIZE 256
//最大长度0x40=64
#define MAX_PAYLOAD_LEN 64
//帧超时时间，100ms
#define FRAME_TIMEOUT_MS 100

uint8_t rx_ring[RX_RING_SIZE];//环形缓冲区
volatile uint16_t rx_write_index = 0;
volatile uint16_t rx_read_index = 0;
uint8_t uart_rx_byte;
uint8_t current_len = 0;
uint8_t current_cmd = 0;
uint8_t payload_index = 0;
uint8_t current_payload[MAX_PAYLOAD_LEN];
uint16_t received_crc;

//统计计数器，方便在ozone中调试
volatile uint32_t frame_ok_count = 0;
volatile uint32_t crc_error_count = 0;
volatile uint32_t length_error_count = 0;
volatile uint32_t timeout_count = 0;
volatile uint32_t header_error_count = 0;
volatile uint32_t ring_overflow_count = 0;
volatile uint32_t last_byte_time = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
typedef enum
{
    WAIT_HEADER_1 = 0,//帧头1
    WAIT_HEADER_2,      //帧头2
    WAIT_LEN,           //长度
    WAIT_CMD,           //命令
    WAIT_PAYLOAD,       //数据本身
    //crc
    WAIT_CRC_LOW,       
    WAIT_CRC_HIGH
} ParserState_t;
ParserState_t parser_state = WAIT_HEADER_1;

uint16_t CRC_MODBUS(uint8_t *data, uint16_t length)
{
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < length; i++)
  {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++)
    {
      if (crc & 0x0001)
      {
        crc >>= 1;
        crc ^= 0xA001;
      }
      else
      {
        crc >>= 1;
      }
    }
  }
  return crc;
}

/*用此函数将接收到的数据放入环形缓冲区*/
void RingBuffer_Push(uint8_t data)
{
  //如果下个位置等于写指针或者两个指针在一头一尾，说明环形缓冲区已满，选择丢弃数据
  if (rx_write_index + 1 == rx_read_index||(rx_write_index + 1 == RX_RING_SIZE && rx_read_index == 0))
    {
      ring_overflow_count++;
      return;
    }
  //将数据放入环形缓冲区指针为写入索引的位置
  rx_ring[rx_write_index] = data;
  //更新写入索引，环形缓冲区满时回绕
  rx_write_index = (rx_write_index + 1) % RX_RING_SIZE;
}

/*用此函数从环形缓冲区中取出数据*/
bool RingBuffer_Pop(uint8_t *data)
{
  if (rx_read_index == rx_write_index)
  {
    return false; //缓冲区为空
  }
  
  *data = rx_ring[rx_read_index];
  //更新读取索引
  rx_read_index = (rx_read_index + 1) % RX_RING_SIZE;
  return true;
}

//接下来实现状态机
/*这个函数用于重置状态，在接收到错误帧或超时的情况下调用*/
void Parser_Reset(void)
{
  parser_state = WAIT_HEADER_1;

  current_len = 0;
  current_cmd = 0;
  payload_index = 0;
  received_crc = 0;
  last_byte_time=0;
}

/*这个函数用于处理接收到的有效帧*/
/*但是在这个例子中，我们只是简单地增加一个计数器来表示接收到的有效帧数量*/
void Handle_Valid_Frame(uint8_t cmd,const uint8_t *payload,uint8_t length)
{
    (void)payload;
    (void)length;
    (void)cmd;

    frame_ok_count++;
}
 
//这个函数用于处理接收到的字节，并根据状态机的当前状态进行相应的操作
void Parser_Process_Byte(uint8_t byte)
{
  switch(parser_state)
  {
    case WAIT_HEADER_1:
      if (byte == 0x5A)
      {
        parser_state = WAIT_HEADER_2;
      }
      break;

    case WAIT_HEADER_2:
      if (byte == 0xA5)
      {
        parser_state = WAIT_LEN;
      }
      else if(byte==0x5A)
      {
        parser_state = WAIT_HEADER_2;
      }
      else
      {
        header_error_count++;
        parser_state = WAIT_HEADER_1;
      }
      break;
    
    case WAIT_LEN:
      current_len = byte;

      if(current_len > MAX_PAYLOAD_LEN||current_len==0)
      {
        length_error_count++;
        Parser_Reset();
      }
      else
      {
        parser_state = WAIT_CMD;
      }
      break;

    case WAIT_CMD:
      current_cmd = byte;
      payload_index = 0;
      parser_state = WAIT_PAYLOAD;
      break;

    case WAIT_PAYLOAD:
      current_payload[payload_index] = byte;
      payload_index++;//更新payload索引

      if(payload_index >= current_len)
      {
        parser_state = WAIT_CRC_LOW;
      }
      break;

    case WAIT_CRC_LOW:
      received_crc = byte;
      parser_state = WAIT_CRC_HIGH;
      break;

    case WAIT_CRC_HIGH:
      received_crc |= (uint16_t)byte << 8;
      //打包数据以计算CRC
      uint8_t crc_date[4 + MAX_PAYLOAD_LEN];
      crc_date[0]=0x5A;
      crc_date[1]=0xA5;
      crc_date[2]=current_len;
      crc_date[3]=current_cmd;
      for(uint8_t i=0;i<current_len;i++)
      {
        crc_date[4+i]=current_payload[i];
      }

      uint16_t calculated_crc;
      calculated_crc = CRC_MODBUS(crc_date, 4 + current_len);

      //比较计算的CRC和接收到的CRC
      if (calculated_crc == received_crc)
      {
        Handle_Valid_Frame(current_cmd, current_payload, current_len);
        Parser_Reset();
      }
      else
      {
        crc_error_count++;
        Parser_Reset();
      }
      break;

      //如果状态机处于未知状态，重置解析器
    default:
      Parser_Reset();
      break;
  }
  //更新最后接收字节的时间
  last_byte_time = HAL_GetTick(); 
}

  /*此函数用于检查接收数据的超时情况，如果在规定时间内没有接收到完整的帧，则重置解析器状态*/
  void Parser_Check_Timeout(void)
  {
    uint32_t current_time = HAL_GetTick();

    //如果状态机不在等待帧头1的状态，并且自上次接收字节以来已经超过了超时时间，则认为接收超时，增加超时计数器并重置解析器
    if (parser_state != WAIT_HEADER_1 && (current_time - last_byte_time) > FRAME_TIMEOUT_MS)
    {
      timeout_count++;
      Parser_Reset();
    }

  }

  void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
  {
    if (huart == &huart1)
    {
      //将接收到的字节放入环形缓冲区
      RingBuffer_Push(uart_rx_byte);
      HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    //取出环形缓冲区中的数据并放入状态机进行处理
    while(RingBuffer_Pop(&uart_rx_byte))
    {
      Parser_Process_Byte(uart_rx_byte);
    }

    //不断检查是否有超时发生
    Parser_Check_Timeout();
    /* USER CODE END WHILE */

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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

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
