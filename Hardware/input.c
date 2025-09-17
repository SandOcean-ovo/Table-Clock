/**
 * @file input.c
 * @brief 输入函数
 * @details 本文件定义了输入功能
 * @author SandOcean
 * @date 2025-08-26
 * @version 1.0
 */

#include "input.h"

/* Private variables ---------------------------------------------------------*/
static Input_Event_Data_t input_event_fifo[INPUT_FIFO_SIZE]; ///< 输入事件的FIFO循环队列
static uint8_t fifo_head = 0;      ///< FIFO队列的写指针
static uint8_t fifo_tail = 0;      ///< FIFO队列的读指针
static uint8_t fifo_count = 0;     ///< FIFO队列中当前的事件数量

static TIM_HandleTypeDef *g_htim_encoder = NULL; ///< 用于编码器的定时器句柄
static TIM_HandleTypeDef *g_htim_scan = NULL;    ///< 用于按键扫描的定时器句柄

static volatile int32_t last_encoder_count = 0; ///< 上一次读取的编码器计数值

static uint32_t system_tick = 0; ///< 由input_tick更新的系统时间戳

///< 返回键对象实例
static Key_t Key_Back = {INPUT_STATE_IDLE, 0, KEY_BCK_GPIO_Port, KEY_BCK_Pin};
///< 确认键对象实例
static Key_t Key_Confirm = {INPUT_STATE_IDLE, 0, KEY_CON_GPIO_Port, KEY_CON_Pin};
///< 编码器按键对象实例
static Key_t Key_Encoder = {INPUT_STATE_IDLE, 0, KEY_EN_GPIO_Port, KEY_EN_Pin};


/* Private function prototypes -----------------------------------------------*/
static uint8_t fifo_push_event(Input_Event_t event, int16_t value);
static uint8_t fifo_pop_event(Input_Event_Data_t *event);
static void input_tick(void);
static void Encoder_Reset(void);
static void Encoder_Update(void);
static void Key_Update(Key_t *key, Input_Event_t press_event);
static void Keys_Update(void);

/* Function implementations --------------------------------------------------*/

/**
 * @brief 将一个新事件推入FIFO队列
 * @param[in] event 事件类型
 * @param[in] value 事件相关的值
 * @return uint8_t
 *      - @retval 1 成功推入。
 *      - @retval 0 队列已满。
 */
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

/**
 * @brief 从FIFO队列中弹出一个事件
 * @param[out] event 用于存储弹出事件的指针
 * @return uint8_t
 *      - @retval 1 成功弹出。
 *      - @retval 0 队列为空。
 */
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

/**
 * @brief 更新内部的系统时间戳
 */
static void input_tick(void)
{
    system_tick = HAL_GetTick();
}

/**
 * @brief 重置编码器计数器
 */
static void Encoder_Reset(void)
{
    if (g_htim_encoder) {
        __HAL_TIM_SET_COUNTER(g_htim_encoder, 0);
    }
    last_encoder_count = 0;
}

/**
 * @brief 检查并更新编码器状态
 * @details 检测编码器计数变化，并将变化量作为事件推入队列。
 */
static void Encoder_Update(void) 
{
    if (!g_htim_encoder) return;

    uint16_t current_count = __HAL_TIM_GET_COUNTER(g_htim_encoder); 
    int16_t delta = (int16_t)(current_count - (uint16_t)last_encoder_count);

    if (delta != 0) {
        fifo_push_event(INPUT_EVENT_ENCODER, delta);
        last_encoder_count = current_count;
    }
}

/**
 * @brief 更新单个按键的状态机
 * @param[in] key 指向要更新的按键对象
 * @param[in] press_event 该按键按下时对应的事件类型
 * @return 无
 */
static void Key_Update(Key_t *key, Input_Event_t press_event)
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

/**
 * @brief 更新所有已定义的按键状态
 */
static void Keys_Update(void)
{
    Key_Update(&Key_Back, INPUT_EVENT_BACK_PRESSED);
    Key_Update(&Key_Confirm, INPUT_EVENT_COMFIRM_PRESSED);
    Key_Update(&Key_Encoder, INPUT_EVENT_ENCODER_PRESSED);
}

void input_scan_timer_irq_handler(TIM_HandleTypeDef *htim)
{
    if (htim == g_htim_scan)
    {
        input_tick();
        Keys_Update();
        Encoder_Update();
    }
}

/**
 * @brief 初始化输入模块
 * @param[in] htim_encoder 编码器定时器句柄
 * @param[in] htim_scan 按键扫描定时器句柄
 * @return 无
 */
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

/**
 * @brief 获取一个输入事件
 * @param[out] event 用于存储事件的指针
 * @return uint8_t
 *      - @retval 1 成功获取事件。
 *      - @retval 0 没有事件。
 */
uint8_t input_get_event(Input_Event_Data_t *event)
{
    return fifo_pop_event(event);
}

/**
 * @brief 获取队列中是否有未处理的输入事件
 * @return uint8_t - 返回队列中未处理事件的个数。
 */
uint8_t input_count_events(void)
{
    return fifo_count;
}

/**
 * @brief 清空输入事件队列
 * @return 无
 */
void input_clear_events(void)
{
    __disable_irq();
    memset(input_event_fifo, 0, sizeof(input_event_fifo));
    fifo_head = 0;
    fifo_tail = 0;
    fifo_count = 0;
    __enable_irq();
}

