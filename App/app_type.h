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

    MENU_CLOCK = 0,      // 主界面 - 时钟显示

    /* 主菜单 */
    MENU_DISPLAY,    // 显示子菜单
    MENU_TIME_SET,       // 时间设置
    MENU_SYSTEM_INFO,    // 系统信息

    /* 显示子菜单 */
    MENU_DISPLAY_LANGUAGE,    // 语言选择
    MENU_DISPLAY_THEME,         // 主题选择
    MENU_DISPLAY_AUTO_OFF,      // 自动关机设置

    /* 语言选择 */
    MENU_LANGUAGE_CN,    // 中文
    MENU_LANGUAGE_EN,    // 英文

    /* 主题选择 */
    MENU_THEME_CLASSIC,    // 经典主题
    MENU_THEME_CYBERPUNK,  // 赛博朋克主题

    /* 自动关机设置 */
    MENU_AUTO_OFF_OFF,    // 关闭自动关机
    MENU_AUTO_OFF_5,      // 5分钟自动关机
    MENU_AUTO_OFF_10,     // 10分钟自动关机
    MENU_AUTO_OFF_20,     // 20分钟自动关机

    /* 时间设置 */
    MENU_TIME_SET_YEAR,    // 设置年份
    MENU_TIME_SET_MONTH,   // 设置月份
    MENU_TIME_SET_DAY,     // 设置日期
    MENU_TIME_SET_HOUR,    // 设置小时
    MENU_TIME_SET_MINUTE,  // 设置分钟
    MENU_TIME_SET_SECOND,  // 设置秒数

} MenuState_t; /* 菜单状态类型 */



typedef struct {
    uint32_t magic_number;
    uint8_t language;
    uint8_t theme;
    uint8_t auto_off;

    uint8_t checksum;
} Settings_t;









#endif /* __APP_TYPE_H */
