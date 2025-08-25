/**
 * @file app_display.c
 * @brief 应用显示函数
 * @details 本文件定义了应用的显示功能
 * @author SandOcean
 * @date 2025-08-25
 * @version 1.0
 */

#include "app_display.h"

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
    const char *week_str[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
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