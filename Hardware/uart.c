#include "uart.h"
#include "stdio.h"
#include "usart.h"
#include "string.h"

#ifdef __GNUC__
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

// 静态变量定义
static uint8_t printf_buffer[PRINTF_DMA_BUFFER_SIZE];
static volatile uint16_t buffer_index = 0; 
static volatile uint8_t dma_busy = 0;       

/**
  * @brief  初始化UART printf DMA发送
  * @param  None
  * @retval None
  */
void UART_Printf_Init(void)
{
    buffer_index = 0;
    dma_busy = 0;
    memset(printf_buffer, 0, PRINTF_DMA_BUFFER_SIZE);
}

/**
  * @brief  刷新缓冲区，使用DMA发送数据 (增加了临界区保护)
  * @param  None
  * @retval None
  */
void UART_Printf_Flush(void)
{
    // 进入临界区，防止在检查和启动DMA之间被中断
    __disable_irq();

    if (buffer_index > 0 && !dma_busy)
    {
        dma_busy = 1;
        // 注意：HAL_UART_Transmit_DMA本身会处理DMA和中断的配置，
        // 我们只需保护我们自己的状态变量。
        HAL_UART_Transmit_DMA(&huart1, printf_buffer, buffer_index);
        buffer_index = 0; // DMA启动后，逻辑缓冲区可以清空
    }
    
    __enable_irq(); // 退出临界区
}

/**
  * @brief  反初始化UART printf DMA发送
  * @param  None
  * @retval None
  */
void UART_Printf_DeInit(void)
{
    // 等待DMA发送完成
    while (dma_busy)
    {
        HAL_Delay(1);
    }
    buffer_index = 0;
    dma_busy = 0;
}

/**
  * @brief  重定向 C 库函数 printf 到 USART (非阻塞版本)
  * @param  ch: 要发送的字符
  * @retval 发送的字符
  */
PUTCHAR_PROTOTYPE
{
    // 如果缓冲区满了，则丢弃该字符 (或者返回错误)
    // 这是非阻塞设计的常见策略
    if (buffer_index >= PRINTF_DMA_BUFFER_SIZE)
    {
        // 可以选择在这里触发一次刷新，尝试清空缓冲区
        UART_Printf_Flush();
        return -1; // 表示失败
    }
    
    // 进入临界区，保护 buffer_index
    __disable_irq();
    printf_buffer[buffer_index++] = (uint8_t)ch;
    __enable_irq();
    
    // 如果是换行符，或者缓冲区半满，可以触发一次刷新
    if (ch == '\n' || buffer_index >= PRINTF_DMA_BUFFER_SIZE / 2)
    {
        UART_Printf_Flush();
    }
    
    return ch;
}

/**
  * @brief  UART DMA发送完成回调函数
  * @param  huart: UART句柄指针
  * @retval None
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        dma_busy = 0;  // 清除DMA忙标志
    }
}






