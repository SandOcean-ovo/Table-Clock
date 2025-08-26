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



typedef struct {
    uint32_t magic_number;
    uint8_t language;
    uint8_t auto_off;

    uint8_t checksum;
} Settings_t;









#endif /* __APP_TYPE_H */
