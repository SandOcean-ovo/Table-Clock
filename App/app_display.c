/**
 * @file app_display.c
 * @brief 应用显示函数
 * @details 本文件定义了应用的显示功能
 * @author SandOcean
 * @date 2025-08-25
 * @version 1.0
 */

#include "app_display.h"

u8g2_t u8g2;

// --- 模块私有函数 ---

void draw_clock_interface(u8g2_t *u8g2)
{
    char buffer[32]; // 用于格式化字符串的缓冲区
    static Time_t current_time;

    DS3231_GetTime(&current_time);
    /* --- 1. 绘制时间 --- */
    // 选择一个大号、清晰的数字字体
    u8g2_SetFont(u8g2, CLOCK_FONT);
    // 格式化时间字符串，例如 "10:08:45"，%02d可以确保数字不足两位时前面补0
    sprintf(buffer, "%02d:%02d:%02d", current_time.hour, current_time.minute, current_time.second);
    // 计算字符串宽度以实现水平居中
    u8g2_uint_t time_width = u8g2_GetStrWidth(u8g2, buffer);
    u8g2_DrawStr(u8g2, (128 - time_width) / 2, 28, buffer);

    /* --- 2. 绘制分割线 --- */
    u8g2_DrawHLine(u8g2, 0, 36, 128);

    /* --- 3. 绘制日期和星期 --- */
    // 选择一个小一点的字体
    u8g2_SetFont(u8g2, DATE_TEMP_FONT);
    const char *week_str[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
    // 绘制星期
    u8g2_DrawStr(u8g2, 2, 50, week_str[current_time.week - 1]); // 注意：week是1-7，数组索引是0-6
    // 绘制日期
    sprintf(buffer, "%04d-%02d-%02d", current_time.year, current_time.month, current_time.day);
    u8g2_uint_t date_width = u8g2_GetStrWidth(u8g2, buffer);
    u8g2_DrawStr(u8g2, (128 - date_width - 2), 50, buffer);

    /* --- 4. 绘制温度 --- */
    sprintf(buffer, "T:%.1fC", DS3231_GetTemperature());
    u8g2_DrawStr(u8g2, 2, 62, buffer);
}

void draw_info_page(u8g2_t *u8g2) 
{
    static uint32_t last_anim_tick = 0;
    static int current_line = 0;      // 当前动画进行到第几行
    const int total_lines = 10;       // 动画总共有多少行/步骤


    // --- 动画结束后的最终页面 ---
    
    /* 1. 标题和Logo */
    u8g2_SetFont(u8g2, u8g2_font_unifont_t_symbols); // 符号字体
    u8g2_DrawGlyph(u8g2, 0, 12, 0x2194); // 一个双向箭头符号作为Logo
    u8g2_SetFont(u8g2, u8g2_font_profont12_tf);
    u8g2_DrawStr(u8g2, 18, 12, APP_NAME);
    u8g2_DrawHLine(u8g2, 0, 15, 128);

    /* 2. 基本信息 */
    // char buffer[40];
    u8g2_SetFont(u8g2, u8g2_font_profont10_tf);
    u8g2_DrawStr(u8g2, 0, 28, "FIRMWARE: " APP_VERSION);
    
    // 使用编译时宏，信息自动更新！
    u8g2_DrawStr(u8g2, 0, 38, "BUILD: " __DATE__);
    u8g2_DrawStr(u8g2, 0, 48, "AUTHOR: " APP_AUTHOR);

    /* 3. 硬件信息 (展示你的技术细节) */
    u8g2_DrawStr(u8g2, 0, 58, "MCU: STM32F103 @72MHz");

    /* 4. GitHub链接 (最重要的部分！) */
    // 制作一个闪烁的提示符，吸引注意力
    if ((HAL_GetTick() / 500) % 2) {
        u8g2_DrawStr(u8g2, 0, 64, ">");
    }
    u8g2_SetFont(u8g2, u8g2_font_u8glib_4_tf); // 用一个很小的字体显示链接
    u8g2_DrawStr(u8g2, 0, 64, "GITHUB.COM/YOUR_USERNAME/YOUR_REPO");

}


// --- 公共函数实现 ---

void app_display_init(void)
{
    u8g2Init(&u8g2);
}