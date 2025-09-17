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

extern bool g_settings_load_failed;

/**
 * @brief 所有初始化函数的整合 
 * @return 无
 */
void app_main_init(void);   

/**
 * @brief 主循环
 * @note 该函数应在主循环中被周期性调用，以处理页面管理和输入事件。 
 * @return 无
 */
void app_main_loop(void);

#endif /* __APP_MAIN_H */
