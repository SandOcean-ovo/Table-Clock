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

// 用于保存定时器句柄的静态指针
static TIM_HandleTypeDef *g_htim_encoder = NULL;
static TIM_HandleTypeDef *g_htim_scan = NULL;

static volatile int32_t last_encoder_count = 0;

static uint32_t system_tick = 0;

static Key_t Key_Back = {INPUT_STATE_IDLE, 0, KEY_BCK_GPIO_Port, KEY_BCK_Pin};
static Key_t Key_Confirm = {INPUT_STATE_IDLE, 0, KEY_CON_GPIO_Port, KEY_CON_Pin};
static Key_t Key_Encoder = {INPUT_STATE_IDLE, 0, KEY_EN_GPIO_Port, KEY_EN_Pin};


// --- 模块私有函数 ---
static uint8_t fifo_push_event(Input_Event_t event, int16_t value)
{
    if (fifo_count >= INPUT_FIFO_SIZE) {
        return 0; // FIFO满
    }
    
    __disable_irq();

    input_event_fifo[fifo_head].event = event;
    input_event_fifo[fifo_head].value = value;
    input_event_fifo[fifo_head].timestamp = system_tick;
    
    fifo_head = (fifo_head + 1) % INPUT_FIFO_SIZE;
    fifo_count++;
    
    __enable_irq();

    return 1;
}

static uint8_t fifo_pop_event(Input_Event_Data_t *event)
{
    if (fifo_count == 0) {
        return 0; // FIFO空
    }
    
    __disable_irq();

    *event = input_event_fifo[fifo_tail];
    fifo_tail = (fifo_tail + 1) % INPUT_FIFO_SIZE;
    fifo_count--;
    
    __enable_irq();
    return 1;
}

void input_tick(void)
{
    system_tick = HAL_GetTick();
}

void Encoder_Reset(void)
{
    if (g_htim_encoder) {
        __HAL_TIM_SET_COUNTER(g_htim_encoder, 0);
    }
    last_encoder_count = 0;
}

void Encoder_Update(void) 
{
    // 使用保存的编码器定时器句柄
    if (!g_htim_encoder) return;

    uint16_t current_count = __HAL_TIM_GET_COUNTER(g_htim_encoder); 
    int16_t delta = (int16_t)(current_count - (uint16_t)last_encoder_count);

    if (delta != 0) {
        fifo_push_event(INPUT_EVENT_ENCODER, delta);
        last_encoder_count = current_count;
    }
}

void Key_Update(Key_t *key, Input_Event_t press_event)
{
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(key->port, key->pin);
    
    switch (key->state) {
        case INPUT_STATE_IDLE:
            if (pin_state == GPIO_PIN_RESET) { // 按键按下
                key->state = INPUT_STATE_DEBOUNCE;
                key->debounce_count = 1;
            }
            break;
        
        case INPUT_STATE_DEBOUNCE:
            if (pin_state == GPIO_PIN_RESET) {
                key->debounce_count++;
                if (key->debounce_count >= INPUT_KEY_DEBOUNCE) {
                    key->state = INPUT_STATE_PRESSED;
                    fifo_push_event(press_event, 0);
                }
            } else {
                // 按键弹起，回到空闲状态
                key->state = INPUT_STATE_IDLE;
                key->debounce_count = 0;
            }
            break;
        
        case INPUT_STATE_PRESSED:
            if (pin_state == GPIO_PIN_SET) { // 按键弹起
                key->state = INPUT_STATE_IDLE;
                key->debounce_count = 0;
            }
            break;
        
        default:
            key->state = INPUT_STATE_IDLE;
            key->debounce_count = 0;
            break;
    }
}

void Keys_Update(void)
{
    Key_Update(&Key_Back, INPUT_EVENT_BACK_PRESSED);
    Key_Update(&Key_Confirm, INPUT_EVENT_COMFIRM_PRESSED);
    Key_Update(&Key_Encoder, INPUT_EVENT_ENCODER_PRESSED);
}

void input_scan_timer_irq_handler(TIM_HandleTypeDef *htim)
{
    if (htim == g_htim_scan)
    {
        // 1. 更新系统时间戳
        input_tick();

        // 2. 扫描所有按键
        Keys_Update();

        // 3. 更新编码器状态
        Encoder_Update();
    }
}

// --- 公共函数实现 ---

void input_init(TIM_HandleTypeDef *htim_encoder, TIM_HandleTypeDef *htim_scan)
{
    g_htim_encoder = htim_encoder;
    g_htim_scan = htim_scan;

    // 初始化FIFO
    memset(input_event_fifo, 0, sizeof(input_event_fifo));
    fifo_head = 0;
    fifo_tail = 0;
    fifo_count = 0;

    input_tick();

    if (g_htim_encoder) {
        HAL_TIM_Encoder_Start(g_htim_encoder, TIM_CHANNEL_ALL);
    }
    if (g_htim_scan) {
        HAL_TIM_Base_Start_IT(g_htim_scan); 
    }

    Encoder_Reset();
}

uint8_t input_get_event(Input_Event_Data_t *event)
{
    return fifo_pop_event(event);
}

