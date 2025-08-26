/**
 * @file input.h
 * @brief 输入头文件
 * @details 本文件定义了输入功能
 * @author SandOcean
 * @date 2025-08-26
 * @version 1.0
 */

#ifndef __INPUT_H
#define __INPUT_H

#include "main.h"
#include "tim.h"
#include "stdint.h"
#include "string.h"

#define INPUT_KEY_DEBOUNCE 2 // 按键消抖扫描次数
#define INPUT_FIFO_SIZE 32 // 按键事件FIFO大小

extern TIM_HandleTypeDef htim1;

typedef enum {
    INPUT_EVENT_NONE = 0,
    INPUT_EVENT_BACK_PRESSED,
    INPUT_EVENT_COMFIRM_PRESSED,
    INPUT_EVENT_ENCODER_PRESSED,
    INPUT_EVENT_ENCODER_UP,
    INPUT_EVENT_ENCODER_DOWN,
} Input_Event_t;

typedef struct {
    Input_Event_t event;
    uint32_t timestamp;
} Input_Event_Data_t;


void input_init(void);

void Encoder_Reset(void);


#endif /* __INPUT_H */