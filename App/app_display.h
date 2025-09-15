/**
 * @file app_display.h
 * @brief 页面管理器和UI框架的头文件
 * @details 定义了页面基类、页面函数指针、全局页面实例以及页面管理器的外部接口。
 *          这是整个UI系统的核心驱动。
 * @author SandOcean
 * @date 2025-09-09
 * @version 1.1
 */

#ifndef __APP_DISPLAY_H
#define __APP_DISPLAY_H

#include "main.h"
#include "app_config.h"
#include "app_type.h"
#include "DS3231.h"
#include "AHT20.h"
#include "u8g2.h"
#include "u8x8.h"
#include "u8g2_stm32_hal.h"
#include "input.h"
#include "string.h"
#include "stdint.h"

/** @defgroup UI_Fonts UI 字体定义 */
/** @{ */
#define CLOCK_FONT u8g2_font_logisoso24_tn /**< 主时钟界面使用的大号数字字体 */
#define DATE_TEMP_FONT u8g2_font_6x10_tf   /**< 用于显示日期、温度等信息的小号字体 */
#define MENU_FONT u8g2_font_ncenB10_tr   /**< 菜单选项使用的字体 */
/** @} */

/**
 * @brief 向前声明页面基类结构体
 * @details 允许在函数指针类型定义中引用 Page_Base 自身。
 */
struct Page_Base;

/** @defgroup Page_Function_Pointers 页面行为函数指针类型 */
/** @{ */

/**
 * @brief 页面进入函数指针类型
 * @param page 指向当前页面实例的指针
 */
typedef void (*Page_Enter_f)(struct Page_Base* page);

/**
 * @brief 页面退出函数指针类型
 * @param page 指向当前页面实例的指针
 */
typedef void (*Page_Exit_f)(struct Page_Base* page);

/**
 * @brief 页面循环逻辑函数指针类型
 * @param page 指向当前页面实例的指针
 */
typedef void (*Page_Loop_f)(struct Page_Base* page);

/**
 * @brief 页面绘制函数指针类型
 * @param page 指向当前页面实例的指针
 * @param u8g2 指向u8g2实例的指针，用于绘图
 * @param x_offset 绘制的X轴偏移量，用于实现动画
 * @param y_offset 绘制的Y轴偏移量，用于实现动画
 */
typedef void (*Page_Draw_f)(struct Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset);

/**
 * @brief 页面动作处理函数指针类型
 * @param page 指向当前页面实例的指针
 * @param u8g2 指向u8g2实例的指针
 * @param action_type 触发的动作类型 (例如，来自input模块的事件)
 */
typedef void (*Page_Action_f)(struct Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event);

/** @} */

/**
 * @brief 页面基类结构体，定义了一个页面的所有行为和属性
 */
typedef struct Page_Base {
    Page_Enter_f  enter;    /**< 进入此页面时调用的函数，用于初始化 */
    Page_Exit_f   exit;     /**< 退出此页面时调用的函数，用于清理资源 */
    Page_Loop_f   loop;     /**< 页面处于活动状态时，在主循环中调用的函数 */
    Page_Draw_f   draw;     /**< 绘制此页面的函数，由管理器在需要刷新时调用 */
    Page_Action_f action;   /**< 处理输入事件或动作的函数 */
    
    const char*   page_name;      /**< 页面的名称，用于调试 */
    struct Page_Base* parent_page;/**< 指向父页面的指针，用于实现“返回”功能 */
    
    uint32_t      refresh_rate_ms;/**< 页面的建议刷新率（毫秒），0表示尽可能快 */
    uint32_t      last_refresh_time;/**< 上次刷新的时间戳 */
} Page_Base;

/** @defgroup Global_Pages 全局页面实例声明 */
/** @{ */
extern Page_Base g_page_main; 
extern Page_Base g_page_main_menu;  
extern Page_Base g_page_display; 
extern Page_Base g_page_info; 
extern Page_Base g_page_time; 
extern Page_Base g_page_language; 
extern Page_Base g_page_auto_off;
/** @} */

/** @defgroup Page_Manager_API 页面管理器外部接口 */
/** @{ */

/**
 * @brief 初始化页面管理器
 * @param[in] u8g2_ptr 指向已初始化的u8g2实例的指针
 */
void Page_Manager_Init(u8g2_t* u8g2_ptr);

/**
 * @brief 页面管理器的主循环函数
 * @details 此函数应在main的while(1)中被反复调用。它负责驱动当前页面的loop和draw。
 */
void Page_Manager_Loop(void);

// /**
//  * @brief 向页面管理器传递一个动作或事件
//  * @param[in] action_type 动作类型，通常来自输入模块的事件枚举
//  */
// void Page_Manager_Action(uint8_t action_type);

/**
 * @brief 切换到指定的页面
 * @details 会调用当前页面的exit函数和新页面的enter函数。
 * @param[in] new_page 指向要切换到的目标页面的指针
 */
void Switch_Page(Page_Base* new_page);

/**
 * @brief 返回到当前页面的父页面
 * @details 如果当前页面没有定义父页面，则此函数无效。
 */
void Go_Back_Page(void);

/** @} */

#endif /* __APP_DISPLAY_H */
