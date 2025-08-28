/**
 * @file input.h
 * @brief 输入处理模块头文件
 * @details 定义了输入事件、按键状态机、数据结构以及外部函数原型。
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

#define INPUT_KEY_DEBOUNCE 2 /**< 按键消抖所需的连续扫描次数 */
#define INPUT_FIFO_SIZE    32 /**< 输入事件FIFO队列的大小 */


/**
 * @brief 定义了所有可能的输入事件类型
 */
typedef enum {
    INPUT_EVENT_NONE = 0,           /**< 无事件 */
    INPUT_EVENT_BACK_PRESSED,       /**< 返回键按下事件 */
    INPUT_EVENT_COMFIRM_PRESSED,    /**< 确认键按下事件 */
    INPUT_EVENT_ENCODER_PRESSED,    /**< 编码器按键按下事件 */
    INPUT_EVENT_ENCODER,            /**< 编码器旋转事件，具体增量在value中 */
} Input_Event_t;

/**
 * @brief 输入事件的数据结构，用于在FIFO中传递
 */
typedef struct {
    Input_Event_t event;    /**< 事件类型, 来自 @ref Input_Event_t */
    int16_t value;          /**< 事件相关的值 (例如编码器旋转的增量) */
    uint32_t timestamp;     /**< 事件发生时的时间戳 (ms) */
} Input_Event_Data_t;

/**
 * @brief 按键状态机的状态定义
 */
typedef enum {
    INPUT_STATE_IDLE = 0,   /**< 空闲状态，按键未按下 */
    INPUT_STATE_DEBOUNCE,   /**< 消抖状态，检测到按键按下，正在确认中 */
    INPUT_STATE_PRESSED,    /**< 按下状态，已确认按键被按下 */
} Key_State_t;

/**
 * @brief 按键对象的数据结构，用于管理单个按键的状态
 */
typedef struct {
    Key_State_t state;          /**< 按键当前的状态机状态, 来自 @ref Key_State_t */
    uint8_t debounce_count;     /**< 用于消抖的计数器 */
    GPIO_TypeDef* port;         /**< 按键连接的GPIO端口 */
    uint16_t pin;               /**< 按键连接的GPIO引脚 */
} Key_t;


/**
 * @brief 初始化输入模块
 * @param[in] htim_encoder 指向用于编码器的定时器句柄
 * @param[in] htim_scan 指向用于按键扫描的定时器句柄
 * @details 该函数会初始化事件FIFO队列，配置并启动编码器和按键扫描所需的定时器。
 *          应该在系统主初始化流程中调用一次。
 */
void input_init(TIM_HandleTypeDef *htim_encoder, TIM_HandleTypeDef *htim_scan);

/**
 * @brief 从FIFO队列中获取一个输入事件
 * @param[out] event 指向 Input_Event_Data_t 结构体的指针，用于存储获取到的事件数据。
 * @return uint8_t 如果成功获取到事件则返回1，如果队列为空则返回0。
 */
uint8_t input_get_event(Input_Event_Data_t *event);


#endif /* __INPUT_H */