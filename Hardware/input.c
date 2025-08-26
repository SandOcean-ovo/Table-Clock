/**
 * @file input.c
 * @brief 输入函数
 * @details 本文件定义了输入功能
 * @author SandOcean
 * @date 2025-08-26
 * @version 1.0
 */

#include "input.h"

static Input_Event_Data_t input_event_fifo[INPUT_FIFO_SIZE];
static uint8_t fifo_head = 0;
static uint8_t fifo_tail = 0;
static uint8_t fifo_count = 0;

static uint32_t system_tick = 0;

// --- 私有函数 ---
static uint8_t fifo_push_event(Input_Event_t event)
{
    if (fifo_count >= INPUT_FIFO_SIZE) {
        return 0; // FIFO满
    }
    
    input_event_fifo[fifo_head].event = event;
    input_event_fifo[fifo_head].timestamp = system_tick;
    
    fifo_head = (fifo_head + 1) % INPUT_FIFO_SIZE;
    fifo_count++;
    
    return 1;
}

static uint8_t fifo_pop_event(Input_Event_Data_t *event)
{
    if (fifo_count == 0) {
        return 0; // FIFO空
    }
    
    *event = input_event_fifo[fifo_tail];
    fifo_tail = (fifo_tail + 1) % INPUT_FIFO_SIZE;
    fifo_count--;
    
    return 1;
}

// --- 公共函数实现 ---

void input_init(void)
{
    // 初始化FIFO
    memset(input_event_fifo, 0, sizeof(input_event_fifo));
    fifo_head = 0;
    fifo_tail = 0;
    fifo_count = 0;

    HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL);
    Encoder_Reset();
    
    // 初始化系统时间
    system_tick = HAL_GetTick();
}