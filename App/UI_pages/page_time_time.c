/**
 * @file page_time_time.c
 * @brief 时间设置页面（老虎机动画）
 * @details 使用老虎机式动画来设置时、分、秒。
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
#define TIME_SLOT_ITEM_COUNT 3
#define TIME_SLOT_ITEM_HEIGHT 22
#define TIME_SLOT_Y_CENTER 32

// --- 2. 定义页面状态和数据结构 ---
typedef enum {
    TIME_STATE_ENTERING,
    TIME_STATE_ZOOMING_IN,
    TIME_STATE_FOCUSED,
    TIME_STATE_ZOOMING_OUT,
    TIME_STATE_SWITCHING,
    TIME_STATE_SLOT_ROLLING,
    TIME_STATE_SHOW_MSG
} Time_Set_State_e;

typedef struct {
    Time_t temp_time;
    int8_t focus_index; // 0=时, 1=分, 2=秒
    Time_Set_State_e state;

    uint32_t anim_start_time;
    float anim_progress;

    float slot_anim_y_offset;
    int16_t slot_anim_direction;
    uint32_t slot_anim_start_time;

    const char* msg_text;
    uint32_t msg_start_time;
} Page_Time_Time_Data_t;

// --- 3. 声明并初始化页面私有数据 ---
static Page_Time_Time_Data_t g_page_data;

// --- 4. 声明本页面的函数 ---
static void Page_Enter(Page_Base* page);
static void Page_Loop(Page_Base* page);
static void Page_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset);
static void Page_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event);

// --- 5. 定义页面全局实例 ---
Page_Base g_page_time_time = {
    .enter = Page_Enter,
    .exit = NULL,
    .loop = Page_Loop,
    .draw = Page_Draw,
    .action = Page_Action,
    .page_name = "TimeSetTime",
    .refresh_rate_ms = 16,
    .last_refresh_time = 0,
};

// --- 6. 函数具体实现 ---
static float lerp(float a, float b, float t) { return a + t * (b - a); }

static void Page_Enter(Page_Base* page) {
    DS3231_GetTime(&g_page_data.temp_time);
    g_page_data.focus_index = 0;
    g_page_data.state = TIME_STATE_ENTERING;
    g_page_data.anim_start_time = HAL_GetTick();
    g_page_data.anim_progress = 0;
    g_page_data.slot_anim_y_offset = 0;
}

static void Page_Loop(Page_Base* page) {
    uint32_t elapsed = HAL_GetTick() - g_page_data.anim_start_time;
    switch (g_page_data.state) {
        case TIME_STATE_ENTERING:
            if (elapsed >= ANIM_DURATION_ENTER) {
                g_page_data.state = TIME_STATE_ZOOMING_IN;
                g_page_data.anim_start_time = HAL_GetTick();
            }
            break;
        case TIME_STATE_ZOOMING_IN: { // 使用花括号，避免编译器警告
            if (elapsed >= ANIM_DURATION_ZOOM) {
                g_page_data.anim_progress = 1.0f;
                g_page_data.state = TIME_STATE_FOCUSED;
            } else {
                g_page_data.anim_progress = (float)elapsed / ANIM_DURATION_ZOOM;
            }
            break;
        }
        case TIME_STATE_ZOOMING_OUT: {
            if (elapsed >= ANIM_DURATION_ZOOM) {
                g_page_data.anim_progress = 0.0f;
                g_page_data.state = TIME_STATE_SWITCHING; // 应该切换到 SWITCHING 状态
            } else {
                g_page_data.anim_progress = 1.0f - ((float)elapsed / ANIM_DURATION_ZOOM);
            }
            break;
        }
        case TIME_STATE_SWITCHING:
            g_page_data.focus_index = (g_page_data.focus_index + 1) % TIME_SLOT_ITEM_COUNT;
            g_page_data.state = TIME_STATE_ZOOMING_IN;
            g_page_data.anim_start_time = HAL_GetTick();
            break;
        case TIME_STATE_SLOT_ROLLING: {
            uint32_t slot_elapsed = HAL_GetTick() - g_page_data.slot_anim_start_time;
            uint32_t slot_duration = 150;
            if (slot_elapsed >= slot_duration) {
                g_page_data.slot_anim_y_offset = 0;
                g_page_data.state = TIME_STATE_FOCUSED;
            } else {
                float progress = (float)slot_elapsed / slot_duration;
                progress = 1.0f - (1.0f - progress) * (1.0f - progress);
                g_page_data.slot_anim_y_offset = g_page_data.slot_anim_direction * TIME_SLOT_ITEM_HEIGHT * (1.0f - progress);
            }
            break;
        }
        case TIME_STATE_FOCUSED: break;
        case TIME_STATE_SHOW_MSG:
            if (HAL_GetTick() - g_page_data.msg_start_time >= 1000) {
                g_page_data.state = TIME_STATE_FOCUSED;
                Go_Back_Page();
            }
            break;
    }
}

static void Page_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset) {
    float p = g_page_data.anim_progress;
    p = p < 0.5 ? 2 * p * p : 1 - pow(-2 * p + 2, 2) / 2;

    const int16_t value_positions_x[] = { 21, 64, 107 };
    const int16_t value_y_small = 36;
    const int16_t label_positions_x[] = { 18, 64, 107 };
    const int16_t label_y_small = 12;
    const int16_t focused_value_x = 64;
    const int16_t focused_value_y = TIME_SLOT_Y_CENTER;
    const int16_t focused_label_x = 20;
    const int16_t focused_label_y = 12;

    const char* labels[] = {"Hour", "Min", "Sec"};
    char str[6];

    for (int i = 0; i < TIME_SLOT_ITEM_COUNT; i++) {
        bool is_focus_target = (i == g_page_data.focus_index);
        int16_t current_value_x, current_value_y, current_label_x, current_label_y;
        const uint8_t *value_font, *label_font;

        if (is_focus_target) {
            current_value_x = lerp(value_positions_x[i], focused_value_x, p);
            current_value_y = lerp(value_y_small, focused_value_y, p);
            current_label_x = lerp(label_positions_x[i], focused_label_x, p);
            current_label_y = lerp(label_y_small, focused_label_y, p);
            value_font = (p > 0.5) ? TIME_FONT_VALUE_LARGE : TIME_FONT_VALUE_SMALL;
            label_font = TIME_FONT_LABEL;
        } else {
            current_value_x = value_positions_x[i];
            current_value_y = value_y_small;
            current_label_x = label_positions_x[i];
            current_label_y = label_y_small;
            value_font = TIME_FONT_VALUE_SMALL;
            label_font = TIME_FONT_LABEL;
        }

        if (!is_focus_target && p > 0.1) continue;

        u8g2_SetFont(u8g2, label_font);
        int16_t label_width = u8g2_GetStrWidth(u8g2, labels[i]);
        u8g2_DrawStr(u8g2, current_label_x - (label_width / 2) + x_offset, current_label_y + y_offset, labels[i]);

        int value = 0;
        if (i == 0) value = g_page_data.temp_time.hour;
        else if (i == 1) value = g_page_data.temp_time.minute;
        else value = g_page_data.temp_time.second;

        u8g2_SetFont(u8g2, value_font);
        sprintf(str, "%02d", value);
        int16_t text_width = u8g2_GetStrWidth(u8g2, str);
        int16_t draw_x = current_value_x - (text_width / 2);

        if (is_focus_target && (g_page_data.state == TIME_STATE_FOCUSED || g_page_data.state == TIME_STATE_SLOT_ROLLING)) {
            int baseline_offset = 6;
            float y_off = g_page_data.slot_anim_y_offset;

            // --- 【关键修复】计算循环边界值 ---
            int value_above, value_below;
            if (i == 0) { // Hour
                value_above = (value == 0) ? 23 : value - 1;
                value_below = (value == 23) ? 0 : value + 1;
            } else { // Minute or Second
                value_above = (value == 0) ? 59 : value - 1;
                value_below = (value == 59) ? 0 : value + 1;
            }
            // --- 修复结束 ---

            // 绘制中心值
            sprintf(str, "%02d", value);
            u8g2_DrawStr(u8g2, draw_x + x_offset, current_value_y + baseline_offset + y_off, str);
            
            // 绘制上方值
            sprintf(str, "%02d", value_above);
            u8g2_DrawStr(u8g2, draw_x + x_offset, current_value_y - TIME_SLOT_ITEM_HEIGHT + baseline_offset + y_off, str);
            
            // 绘制下方值
            sprintf(str, "%02d", value_below);
            u8g2_DrawStr(u8g2, draw_x + x_offset, current_value_y + TIME_SLOT_ITEM_HEIGHT + baseline_offset + y_off, str);
            
            int16_t arrow_width = u8g2_GetStrWidth(u8g2, ">");
            u8g2_DrawStr(u8g2, draw_x - arrow_width - 10 + x_offset, current_value_y + baseline_offset + y_offset, ">");
        } else {
            int baseline_offset = 5;
            u8g2_DrawStr(u8g2, draw_x + x_offset, current_value_y + baseline_offset, str);
        }
    }

    if (g_page_data.state == TIME_STATE_SHOW_MSG) {
        u8g2_SetFont(u8g2, PROMPT_FONT);
        uint16_t msg_w = u8g2_GetStrWidth(u8g2, g_page_data.msg_text);
        uint16_t box_w = msg_w + 10;
        uint16_t box_h = 16;
        uint16_t box_x = (u8g2_GetDisplayWidth(u8g2) - box_w) / 2;
        uint16_t box_y = (u8g2_GetDisplayHeight(u8g2) - box_h) / 2;
        u8g2_SetDrawColor(u8g2, 0);
        u8g2_DrawBox(u8g2, box_x, box_y, box_w, box_h);
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawFrame(u8g2, box_x, box_y, box_w, box_h);
        u8g2_DrawStr(u8g2, box_x + 5, box_y + 12, g_page_data.msg_text);
    }
}

static void Page_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event) {
    if (g_page_data.state != TIME_STATE_FOCUSED) {
        if (event->event == INPUT_EVENT_BACK_PRESSED) Go_Back_Page();
        return;
    }

    switch (event->event) {
        case INPUT_EVENT_ENCODER: {
            switch (g_page_data.focus_index) {
                case 0: // 时
                    g_page_data.temp_time.hour = (g_page_data.temp_time.hour + event->value + 24) % 24;
                    break;
                case 1: // 分
                    g_page_data.temp_time.minute = (g_page_data.temp_time.minute + event->value + 60) % 60;
                    break;
                case 2: // 秒
                    g_page_data.temp_time.second = (g_page_data.temp_time.second + event->value + 60) % 60;
                    break;
            }
            g_page_data.state = TIME_STATE_SLOT_ROLLING;
            g_page_data.slot_anim_direction = (event->value > 0) ? -1 : 1;
            g_page_data.slot_anim_start_time = HAL_GetTick();
            g_page_data.slot_anim_y_offset = g_page_data.slot_anim_direction * TIME_SLOT_ITEM_HEIGHT;
            break;
        }
        case INPUT_EVENT_ENCODER_PRESSED:
            g_page_data.state = TIME_STATE_ZOOMING_OUT;
            g_page_data.anim_start_time = HAL_GetTick();
            break;
        case INPUT_EVENT_COMFIRM_PRESSED:
            Time_t now;
            DS3231_GetTime(&now);
            now.hour = g_page_data.temp_time.hour;
            now.minute = g_page_data.temp_time.minute;
            now.second = g_page_data.temp_time.second;
            DS3231_SetTime(&now);
            g_page_data.msg_text = "Time Saved!";
            g_page_data.state = TIME_STATE_SHOW_MSG;
            g_page_data.msg_start_time = HAL_GetTick();
            break;
        case INPUT_EVENT_BACK_PRESSED:
            Go_Back_Page();
            break;
        default:
            break;
    }
}