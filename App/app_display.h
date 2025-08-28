/**
 * @file app_display.h
 * @brief 应用显示头文件
 * @details 本文件定义了应用的显示功能
 * @author SandOcean
 * @date 2025-08-25
 * @version 1.0
 */

#ifndef __APP_DISPLAY_H
#define __APP_DISPLAY_H

#include "main.h"
#include "app_config.h"
#include "app_type.h"
#include "DS3231.h"
#include "u8g2.h"
#include "u8x8.h"
#include "string.h"
#include "stdint.h"

#define CLOCK_FONT u8g2_font_logisoso24_tn
#define DATE_TEMP_FONT u8g2_font_6x10_tf

extern u8g2_t u8g2;

void app_display_init(void);

void draw_info_page(u8g2_t *u8g2);

void draw_clock_interface(u8g2_t *u8g2);


#endif /* __APP_DISPLAY_H */
