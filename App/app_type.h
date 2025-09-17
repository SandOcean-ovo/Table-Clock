/**
 * @file      app_type.h
 * @brief     应用类型头文件
 * @details   本文件定义了应用层使用的数据结构
 * @author    SandOcean
 * @date      2025-08-25
 * @version   1.0
 * @copyright Copyright (c) 2025 SandOcean
 */

#ifndef __APP_TYPE_H
#define __APP_TYPE_H

#include "main.h"
#include "input.h"
#include "u8g2.h"
#include "stdint.h"
#include <stdbool.h>

// 向前声明UI管理器结构体，以便页面函数可以接收它作为参数
struct UI_Manager_s; 

/**
 * @brief UI页面的函数指针定义
 * @param manager 指向UI管理器的指针
 * @param event 指向输入事件的指针 (仅用于input函数)
 */
typedef void (*ui_page_func_t)(struct UI_Manager_s* manager);
typedef void (*ui_input_func_t)(struct UI_Manager_s* manager, const Input_Event_Data_t* event);

/**
 * @brief 定义了一个UI页面对象
 * @details 每个UI界面都由一个这样的结构体来描述其行为
 */
typedef struct {
    ui_page_func_t on_enter;    // 进入此页面时调用的函数 (用于初始化)
    ui_page_func_t on_exit;     // 退出此页面时调用的函数 (用于清理)
    ui_page_func_t draw;        // 绘制此页面的函数 (每帧调用)
    ui_input_func_t process_input; // 处理此页面输入的函数
} UI_Page_t;

typedef enum {
    NEVER = 0,
    TIME_30S,
    TIME_1MIN,
    TIME_5MIN,
    TIME_10MIN,
} Auto_Off;

typedef struct {
    uint32_t magic_number;
    uint8_t language;
    Auto_Off auto_off;
    bool dst_enabled;

    uint8_t checksum;
} Settings_t;




#endif /* __APP_TYPE_H */
