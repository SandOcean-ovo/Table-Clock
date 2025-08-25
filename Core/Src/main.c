/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
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
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include "uart.h"
#include "u8g2.h"
#include "u8x8.h"
#include "u8g2_stm32_hal.h"
#include "DS3231.h"

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

/* USER CODE BEGIN PV */
u8g2_t u8g2;

Time_t current_time;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void draw_clock_interface(u8g2_t *u8g2, Time_t *time);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  UART_Printf_Init();
  // 初始化OLED
	u8g2Init(&u8g2);
  DS3231_Init(&hi2c1);
   
  uint8_t first_run_flag = AT24C32_ReadByte(0x0000);
  if (first_run_flag != 0xAA) {
      // 如果是第一次运行
      Time_t initial_time = {17, 42, 20, 2025, 8, 25, 1}; 
      DS3231_SetTime(&initial_time);
        
      // 在EEPROM中写入标志位，表示已经初始化过了
      AT24C32_WriteByte(0x0000, 0xAA); 
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

        /* --- 逻辑处理 --- */
        // 从DS3231获取当前时间
        DS3231_GetTime(&current_time);
        
        // 从DS3231获取当前温度
        float temperature = DS3231_GetTemperature();

        // 读写EEPROM示例：保存一个开机次数计数器


        /* --- 显示处理 --- */
        char buffer[32];
        u8g2_FirstPage(&u8g2);
        do {
            // 使用从RTC读到的current_time来绘制时钟界面
            draw_clock_interface(&u8g2, &current_time);

            // 在屏幕底部显示温度和开机次数
            u8g2_SetFont(&u8g2, u8g2_font_5x7_tf);
            sprintf(buffer, "T:%.2fC", temperature);
            u8g2_DrawStr(&u8g2, 2, 62, buffer);

        } while (u8g2_NextPage(&u8g2));
        
        HAL_Delay(1000); // 每秒刷新一次


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

/* USER CODE BEGIN 4 */
void draw_clock_interface(u8g2_t *u8g2, Time_t *time)
{
    char buffer[32]; // 用于格式化字符串的缓冲区

    /* --- 1. 绘制时间 --- */
    // 选择一个大号、清晰的数字字体
    u8g2_SetFont(u8g2, u8g2_font_logisoso24_tn);
    // 格式化时间字符串，例如 "10:08:45"，%02d可以确保数字不足两位时前面补0
    sprintf(buffer, "%02d:%02d:%02d", time->hour, time->minute, time->second);
    // 计算字符串宽度以实现水平居中
    u8g2_uint_t time_width = u8g2_GetStrWidth(u8g2, buffer);
    u8g2_DrawStr(u8g2, (128 - time_width) / 2, 28, buffer);

    /* --- 2. 绘制分割线 --- */
    u8g2_DrawHLine(u8g2, 0, 36, 128);

    /* --- 3. 绘制日期和星期 --- */
    // 选择一个小一点的字体
    u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
    const char *week_str[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    // 绘制星期
    u8g2_DrawStr(u8g2, 2, 50, week_str[time->week]);
    // 绘制日期
    sprintf(buffer, "%04d-%02d-%02d", time->year, time->month, time->day);
    u8g2_uint_t date_width = u8g2_GetStrWidth(u8g2, buffer);
    u8g2_DrawStr(u8g2, (128 - date_width - 2), 50, buffer);


}
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
