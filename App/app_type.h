/**
 * @file app_type.h
 * @brief 应用类型头文件
 * @details 本文件定义了应用层使用的数据结构
 * @author SandOcean
 * @date 2025-08-25
 * @version 1.0
 */

#ifndef __APP_TYPE_H
#define __APP_TYPE_H

#include "main.h"
#include "input.h"
#include "u8g2.h"
#include "stdint.h"


typedef enum {
    // --- 主界面 ---
    UI_STATE_HOME = 0,

    // --- 一级菜单 ---
    UI_STATE_MAIN_MENU,

    // --- 二级菜单 ---
    UI_STATE_DISPLAY_MENU,
    UI_STATE_TIME_MENU,
    UI_STATE_INFO_PAGE, // 信息页不是菜单，但也是一个独立状态

    // --- 三级菜单/设置页 ---
    UI_STATE_DISPLAY_LANGUAGE,
    UI_STATE_DISPLAY_AUTOSLEEP,
    UI_STATE_TIME_SET_DATE,
    UI_STATE_TIME_SET_TIME,
    UI_STATE_TIME_SET_DST,

    // --- 关键：通用的动画过渡状态 ---
    UI_STATE_TRANSITION

} UI_State_t;

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

/**
 * @brief 定义了所有支持的UI过渡动画类型
 */
typedef enum {
    ANIM_TYPE_NONE = 0,         /**< 无动画，瞬间切换 */
    ANIM_TYPE_ZOOM_IN,          /**< 从右下角放大进入 */
    ANIM_TYPE_ZOOM_OUT,         /**< 缩小到右下角退出 */
    ANIM_TYPE_SLIDE_LEFT,       /**< 从右向左滑动进入 */
    ANIM_TYPE_SLIDE_RIGHT,      /**< 从左向右滑动进入 (通常用于返回) */
    ANIM_TYPE_SLIDE_UP,         /**< 从下向上滑动进入 */
    ANIM_TYPE_SLIDE_DOWN,       /**< 从上向下滑动退出 */
    ANIM_TYPE_FADE,             /**< 淡入淡出 (在单色屏上较难实现，但可以保留) */
} AnimationType_t;

/**
 * @brief UI动画状态
 */
typedef struct {
    uint8_t is_active;          // 动画是否正在进行
    uint32_t start_time;        // 动画开始的时间戳
    uint32_t duration;          // 动画总时长 (ms)
    const UI_Page_t* from_page; // 动画来源页面
    const UI_Page_t* to_page;   // 动画目标页面
    AnimationType_t type;     // 动画类型
} UI_Animation_t;

/**
 * @brief UI管理器，管理整个UI的状态
 */
typedef struct UI_Manager_s {
    u8g2_t* u8g2;               // 指向U8G2实例的指针
    const UI_Page_t* current_page; // 指向当前页面的指针
    UI_Animation_t animation;   // 动画状态
    int8_t menu_current_option; // 当前菜单的选中项
    int8_t menu_top_option;     // 当前菜单显示在顶部的选项 (用于滚动)
    void* context;              // 通用上下文指针，用于页面状态
} UI_Manager_t;



/** @brief 定义了一个老虎机式选择器的上下文结构体
 */
typedef struct {
    float currentValue;
    float animatedValue;
    int minValue;
    int maxValue;
    uint8_t isWrapping;
} Picker_Context_t;

typedef struct {
    uint32_t magic_number;
    uint8_t language;
    uint8_t auto_off;

    uint8_t checksum;
} Settings_t;









#endif /* __APP_TYPE_H */
