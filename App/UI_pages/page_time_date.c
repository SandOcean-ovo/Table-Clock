/**
 * @file page_time_date.c
 * @brief 日期设置页面（老虎机动画）
 * @details 使用老虎机式动画来设置年、月、日。
 * @author SandOcean
 * @date 2025-09-17
 * @version 1.0
 */

#include "app_display.h"
#include "app_config.h"
#include "input.h"
#include "DS3231.h" 
#include <stdio.h>
#include <stdbool.h>

// --- 1. 页面私有定义 ---
#define SLOT_ITEM_COUNT 3
#define SLOT_ITEM_HEIGHT 22 // 每个数字占据的高度
#define SLOT_Y_CENTER 32    // 聚焦时老虎机的中心

// --- 2. 定义页面状态和数据结构 ---
typedef enum {
    DATE_STATE_ENTERING,      // 0.5秒初始显示
    DATE_STATE_ZOOMING_IN,    // 放大动画
    DATE_STATE_FOCUSED,       // 聚焦交互
    DATE_STATE_ZOOMING_OUT,   // 缩小动画   
    DATE_STATE_SWITCHING,     // 准备切换
    DATE_STATE_SLOT_ROLLING,   // 老虎机滚动动画
    DATE_STATE_SHOW_MSG,
} Date_Set_State_e;

typedef struct {
    Page_Base base; // 必须包含基类作为第一个成员
    // 私有数据
    Time_t temp_date;      // 用于编辑的临时日期数据
    int8_t focus_index;             // 当前焦点: 0=年, 1=月, 2=日
    Date_Set_State_e state;         // 页面动画状态

    // --- 通用动画变量 ---
    uint32_t anim_start_time;
    float anim_progress; // 0.0 to 1.0

    // --- 老虎机滚动动画变量 ---
    float slot_anim_y_offset;
    int16_t slot_anim_direction;
    uint32_t slot_anim_start_time;

    bool should_save_on_exit;       // 退出时是否保存更改

    const char* msg_text;
    uint32_t msg_start_time;
} Page_Time_Date_Data_t;

// --- 3. 声明并初始化页面私有数据 ---
static Page_Time_Date_Data_t g_page_data;

// --- 4. 声明本页面的函数 ---
static void Page_Enter(Page_Base* page);
static void Page_Exit(Page_Base* page);
static void Page_Loop(Page_Base* page);
static void Page_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset);
static void Page_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event);

// --- 5. 定义页面全局实例 ---
Page_Base g_page_time_date = {
    .enter = Page_Enter,
    .exit = NULL,
    .loop = Page_Loop,
    .draw = Page_Draw,
    .action = Page_Action,
    .page_name = "DateSet",
    .refresh_rate_ms = 16, // 动画需要高刷新率
    .last_refresh_time = 0,
};

// --- 6. 函数具体实现 ---

// 辅助函数：线性插值
static float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// 辅助函数：判断是否为闰年
static bool is_leap_year(uint16_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// 辅助函数：获取指定月份的最大天数
static uint8_t get_max_days_in_month(uint16_t year, uint8_t month) {
    if (month == 2) {
        return is_leap_year(year) ? 29 : 28;
    } else if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    } else {
        return 31;
    }
}

/**
 * @brief 页面进入函数
 */
static void Page_Enter(Page_Base* page) {
    // 从DS3231获取当前日期，存入临时变量
    DS3231_GetTime(&g_page_data.temp_date);
    
    g_page_data.focus_index = 0;
    g_page_data.state = DATE_STATE_ENTERING; // 初始状态
    g_page_data.anim_start_time = HAL_GetTick();
    g_page_data.anim_progress = 0;
    g_page_data.slot_anim_y_offset = 0;
    g_page_data.should_save_on_exit = false; 
}

/**
 * @brief 页面循环逻辑函数 (驱动动画)
 */
static void Page_Loop(Page_Base* page) {
    uint32_t elapsed = HAL_GetTick() - g_page_data.anim_start_time;

    switch (g_page_data.state) {
        case DATE_STATE_ENTERING:
            if (elapsed >= ANIM_DURATION_ENTER) {
                // 初始停留结束，开始放大动画
                g_page_data.state = DATE_STATE_ZOOMING_IN;
                g_page_data.anim_start_time = HAL_GetTick();
            }
            break;

        case DATE_STATE_ZOOMING_IN: {
            if (elapsed >= ANIM_DURATION_ZOOM) {
                g_page_data.anim_progress = 1.0f;
                g_page_data.state = DATE_STATE_FOCUSED; // 动画结束，进入聚焦状态
            } else {
                g_page_data.anim_progress = (float)elapsed / ANIM_DURATION_ZOOM;
            }
            break;
        }

        case DATE_STATE_ZOOMING_OUT: {
            if (elapsed >= ANIM_DURATION_ZOOM) {
                g_page_data.anim_progress = 0.0f;
                g_page_data.state = DATE_STATE_SWITCHING; // 缩小完成，准备切换
            } else {
                // 进度从1.0降到0.0
                g_page_data.anim_progress = 1.0f - ((float)elapsed / ANIM_DURATION_ZOOM);
            }
            break;
        }
        
        case DATE_STATE_SWITCHING:
            // 切换焦点并立即开始放大
            g_page_data.focus_index = (g_page_data.focus_index + 1) % SLOT_ITEM_COUNT;
            g_page_data.state = DATE_STATE_ZOOMING_IN;
            g_page_data.anim_start_time = HAL_GetTick();
            break;

        case DATE_STATE_SLOT_ROLLING: {
            uint32_t slot_elapsed = HAL_GetTick() - g_page_data.slot_anim_start_time;
            uint32_t slot_duration = 150;
            if (slot_elapsed >= slot_duration) {
                g_page_data.slot_anim_y_offset = 0;
                g_page_data.state = DATE_STATE_FOCUSED;
            } else {
                float progress = (float)slot_elapsed / slot_duration;
                progress = 1.0f - (1.0f - progress) * (1.0f - progress); // ease-out
                g_page_data.slot_anim_y_offset = g_page_data.slot_anim_direction * SLOT_ITEM_HEIGHT * (1.0f - progress);
            }
            break;
        }

        case DATE_STATE_FOCUSED:
            // 静态，无事可做
            break;

        case DATE_STATE_SHOW_MSG:
        if (HAL_GetTick() - g_page_data.msg_start_time >= 1000) {
            // 1秒后返回上一页
            g_page_data.state = DATE_STATE_FOCUSED; // 恢复状态
            Go_Back_Page();
        }
        break;
    }
}

/**
 * @brief 页面绘制函数
 */
static void Page_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset) {
    // 动画进度 
    float p = g_page_data.anim_progress;
    p = p < 0.5 ? 2 * p * p : 1 - pow(-2 * p + 2, 2) / 2; // ease-in-out

    // 并列视图下，数值的中心X坐标
    const int16_t value_positions_x[] = { 21, 64, 107 };
    const int16_t value_y_small = 36;

    // 并列视图下，标签的中心X坐标 
    const int16_t label_positions_x[] = { 18, 64, 107 };
    const int16_t label_y_small = 12;

    // 聚焦视图下的坐标
    const int16_t focused_value_x = 64; 
    const int16_t focused_value_y = SLOT_Y_CENTER;
    const int16_t focused_label_x = 12; 
    const int16_t focused_label_y = 12;

    const char* labels[] = {"Year", "Mon", "Day"};
    char str[6];

    for (int i = 0; i < SLOT_ITEM_COUNT; i++) {
        bool is_focus_target = (i == g_page_data.focus_index);
        
        int16_t current_value_x, current_value_y, current_label_x, current_label_y;
        const uint8_t *value_font, *label_font;

        if (is_focus_target) {
            // --- 对标签和数值分别进行插值 ---
            current_value_x = lerp(value_positions_x[i], focused_value_x, p);
            current_value_y = lerp(value_y_small, focused_value_y, p);
            
            current_label_x = lerp(label_positions_x[i], focused_label_x, p);
            current_label_y = lerp(label_y_small, focused_label_y, p);

            // 字体大小根据动画进度变化
            value_font = (p > 0.5) ? DATE_FONT_VALUE_LARGE : DATE_FONT_VALUE_SMALL;
            label_font = DATE_FONT_LABEL;
        } else {
            // 非聚焦项使用并列视图的固定坐标
            current_value_x = value_positions_x[i];
            current_value_y = value_y_small;
            current_label_x = label_positions_x[i];
            current_label_y = label_y_small;
            value_font = DATE_FONT_VALUE_SMALL;
            label_font = DATE_FONT_LABEL;
        }

        // 在放大动画中，让非聚焦项淡出 (可选，但效果好)
        if (!is_focus_target && p > 0.1) {
            // u8g2没有透明度，我们可以通过不绘制来实现淡出效果
        } else {
            // --- 绘制标签 ---
            u8g2_SetFont(u8g2, label_font);
            int16_t label_width = u8g2_GetStrWidth(u8g2, labels[i]);
            u8g2_DrawStr(u8g2, current_label_x - (label_width / 2) + x_offset, current_label_y + y_offset, labels[i]);
            
            // --- 准备绘制数值 ---
            int value = 0;
            const char* format = "%02d";
            if (i == 0) { value = g_page_data.temp_date.year; format = "%04d"; }
            else if (i == 1) { value = g_page_data.temp_date.month; }
            else { value = g_page_data.temp_date.day; }

            u8g2_SetFont(u8g2, value_font);
            sprintf(str, format, value);
            int16_t text_width = u8g2_GetStrWidth(u8g2, str);
            int16_t draw_x = current_value_x - (text_width / 2);

            if (is_focus_target && (g_page_data.state == DATE_STATE_FOCUSED || g_page_data.state == DATE_STATE_SLOT_ROLLING)) {
                // --- 绘制聚焦状态的老虎机 ---
                int baseline_offset = 6;
                float y_off = g_page_data.slot_anim_y_offset;

                // --- 【关键修复】计算循环边界值 ---
                int value_above, value_below;
                if (i == 0) { // Year
                    value_above = (value == 2000) ? 2099 : value - 1;
                    value_below = (value == 2099) ? 2000 : value + 1;
                } else if (i == 1) { // Month
                    value_above = (value == 1) ? 12 : value - 1;
                    value_below = (value == 12) ? 1 : value + 1;
                } else { // Day
                    uint8_t max_days = get_max_days_in_month(g_page_data.temp_date.year, g_page_data.temp_date.month);
                    value_above = (value == 1) ? max_days : value - 1;
                    value_below = (value == max_days) ? 1 : value + 1;
                }

                // 绘制中心值
                sprintf(str, format, value);
                u8g2_DrawStr(u8g2, draw_x + x_offset, current_value_y + baseline_offset + y_off, str);
                
                // 绘制上方值
                sprintf(str, format, value_above);
                u8g2_DrawStr(u8g2, draw_x + x_offset, current_value_y - SLOT_ITEM_HEIGHT + baseline_offset + y_off, str);
                
                // 绘制下方值
                sprintf(str, format, value_below);
                u8g2_DrawStr(u8g2, draw_x + x_offset, current_value_y + SLOT_ITEM_HEIGHT + baseline_offset + y_off, str);
                
                
                int16_t arrow_width = u8g2_GetStrWidth(u8g2, ">");
                int16_t arrow_x = draw_x - arrow_width - 10;
                int16_t arrow_y = current_value_y + baseline_offset; 
                u8g2_DrawStr(u8g2, arrow_x + x_offset, arrow_y + y_offset, ">");

            } else {
                // --- 绘制并列或动画中的数值 ---
                int baseline_offset = 5; 
                u8g2_DrawStr(u8g2, draw_x + x_offset, current_value_y + baseline_offset, str);
            }
        }
    }
    if (g_page_data.state == DATE_STATE_SHOW_MSG) {
        u8g2_SetFont(u8g2, PROMPT_FONT);
        uint16_t msg_w = u8g2_GetStrWidth(u8g2, g_page_data.msg_text);
        uint16_t box_w = msg_w + 10;
        uint16_t box_h = 16;
        uint16_t box_x = (u8g2_GetDisplayWidth(u8g2) - box_w) / 2;
        uint16_t box_y = (u8g2_GetDisplayHeight(u8g2) - box_h) / 2;
        
        // 绘制背景和边框
        u8g2_SetDrawColor(u8g2, 0);
        u8g2_DrawBox(u8g2, box_x, box_y, box_w, box_h);
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawFrame(u8g2, box_x, box_y, box_w, box_h);
        
        // 绘制文字
        u8g2_DrawStr(u8g2, box_x + 5, box_y + 12, g_page_data.msg_text);
    }
}

/**
 * @brief 页面输入事件处理函数
 */
static void Page_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event) {
    // 只有在聚焦状态才接受输入
    if (g_page_data.state != DATE_STATE_FOCUSED) {
        return;
    }

    switch (event->event) {
        case INPUT_EVENT_ENCODER: {
            // 【修改】使用新的边界检查逻辑
            switch (g_page_data.focus_index) {
                case 0: // 年
                    g_page_data.temp_date.year += event->value;
                    if (g_page_data.temp_date.year > 2099) g_page_data.temp_date.year = 2000;
                    if (g_page_data.temp_date.year < 2000) g_page_data.temp_date.year = 2099;
                    break;
                case 1: // 月
                    g_page_data.temp_date.month += event->value;
                    if (g_page_data.temp_date.month > 12) g_page_data.temp_date.month = 1;
                    if (g_page_data.temp_date.month < 1) g_page_data.temp_date.month = 12;
                    break;
                case 2: // 日
                { // 使用花括号创建局部作用域
                    uint8_t max_days = get_max_days_in_month(g_page_data.temp_date.year, g_page_data.temp_date.month);
                    g_page_data.temp_date.day += event->value;
                    if (g_page_data.temp_date.day > max_days) g_page_data.temp_date.day = 1;
                    if (g_page_data.temp_date.day < 1) g_page_data.temp_date.day = max_days;
                    break;
                }
            }
            
            // 【新增】在修改年月后，修正日期值
            // 例如，从3月31日切换到2月时，日期应自动变为28或29
            uint8_t max_days_after_change = get_max_days_in_month(g_page_data.temp_date.year, g_page_data.temp_date.month);
            if (g_page_data.temp_date.day > max_days_after_change) {
                g_page_data.temp_date.day = max_days_after_change;
            }

            // 启动老虎机滚动动画
            g_page_data.state = DATE_STATE_SLOT_ROLLING;
            g_page_data.slot_anim_direction = (event->value > 0) ? -1 : 1;
            g_page_data.slot_anim_start_time = HAL_GetTick();
            g_page_data.slot_anim_y_offset = g_page_data.slot_anim_direction * SLOT_ITEM_HEIGHT;
            break;
        }
        case INPUT_EVENT_ENCODER_PRESSED:
            // 启动缩小动画，为切换焦点做准备
            g_page_data.state = DATE_STATE_ZOOMING_OUT;
            g_page_data.anim_start_time = HAL_GetTick();
            break;

        case INPUT_EVENT_COMFIRM_PRESSED:
            // 确认并保存设置
            Time_t now; 
            DS3231_GetTime(&now);
            now.year = g_page_data.temp_date.year;
            now.month = g_page_data.temp_date.month;
            now.day = g_page_data.temp_date.day;
            DS3231_SetTime(&now); //确保时间不变，只改日期
            g_page_data.msg_text = "Date Saved!";
            g_page_data.state = DATE_STATE_SHOW_MSG;
            g_page_data.msg_start_time = HAL_GetTick();
            break;
        case INPUT_EVENT_BACK_PRESSED:
            // 不保存，直接返回
            Go_Back_Page();
            break;
        default:
            break;
    }
}