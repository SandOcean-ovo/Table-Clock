#ifndef __UART_H
#define __UART_H

#include "main.h"

// DMA发送缓冲区大小
#define PRINTF_DMA_BUFFER_SIZE 256

// 函数声明
void UART_Printf_Init(void);
void UART_Printf_Flush(void);
void UART_Printf_DeInit(void);

#endif
