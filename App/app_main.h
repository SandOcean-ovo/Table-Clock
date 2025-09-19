/**
 * @file      app_main.h
 * @brief     应用主函数
 * @details   本文件声明了应用的主函数
 * @author    SandOcean
 * @date      2025-09-17
 * @version   1.0
 * @copyright Copyright (c) 2025 SandOcean
 */

#ifndef __APP_MAIN_H
#define __APP_MAIN_H

#include "app_config.h"
#include "app_type.h"
#include "app_display.h"
#include "app_settings.h"

#include "DS3231.h"
#include "AHT20.h"
#include "input.h"
#include "u8g2_stm32_hal.h"

/**
 * @brief 指示设置加载是否失败的全局标志
 * @details 如果 `app_settings_load` 函数返回 `false`，此标志将被设置为 `true`，
 *          主页面 (`page_main`) 会根据此标志显示错误提示。
 */
extern bool g_settings_load_failed;

/**
 * @brief 应用主初始化函数
 * @details 此函数封装了所有硬件和软件模块的初始化过程，包括：
 *          - DS3231 RTC模块
 *          - AHT20 温湿度传感器
 *          - u8g2 显示库
 *          - 输入设备 (旋钮编码器)
 *          - 页面管理器
 *          - 加载应用设置
 * @return 无
 */
void app_main_init(void);   

/**
 * @brief 应用主循环函数
 * @details 该函数应在主循环中被周期性调用，以处理页面管理和输入事件。
 * @return 无
 */
void app_main_loop(void);

#endif /* __APP_MAIN_H */
