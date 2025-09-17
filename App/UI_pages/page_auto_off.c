/**
 * @file page_auto_off.c
 * @brief 自动熄屏设置页面
 * @details 本文件定义了“自动熄屏”设置菜单，实现了列表滚动和高亮框移动的交互效果。
 * @author SandOcean
 * @date 2025-09-17
 * @version 1.2
 */

#include "app_display.h"
#include "input.h"
#include "app_config.h" 
#include "app_settings.h"

// --- 1. 页面私有定义 ---
#define AUTO_OFF_ITEM_COUNT 5
#define AUTO_OFF_ITEM_HEIGHT 16
#define AUTO_OFF_LEFT_X 5
#define AUTO_OFF_WIDTH 118
#define VISIBLE_ITEMS 4 // 屏幕上最多可见4个项目
#define LIST_TOP_Y 0    // 列表区域的起始Y坐标

// 菜单动画状态
typedef enum {
    AUTO_OFF_STATE_IDLE,
    AUTO_OFF_STATE_ANIMATING_HIGHLIGHT, // 仅移动高亮框
    AUTO_OFF_STATE_ANIMATING_LIST,      // 滚动整个列表
    AUTO_OFF_STATE_SHOW_MSG             // 显示反馈信息
} Auto_Off_State_e;

// --- 2. 定义页面私有数据结构体 ---
typedef struct {
    int8_t selected_index;      // 绝对选中项索引 (0 to ITEM_COUNT-1)
    int8_t viewport_top_index;  // 可视区域顶部的项目索引
    Auto_Off_State_e state;
    
    float anim_current_y;       // 动画当前Y值 (可以是高亮框或列表)
    int16_t anim_start_y;
    int16_t anim_target_y;
    uint32_t anim_start_time;
    uint32_t anim_duration;

    uint32_t msg_start_time;
    const char* msg_text;
} Page_Auto_Off_Data_t;

// --- 3. 声明并初始化页面私有数据 ---
static Page_Auto_Off_Data_t g_page_auto_off_data;

// 菜单项的字符串
static const char* menu_items[AUTO_OFF_ITEM_COUNT] = {
    "Never", "30s", "1min", "5min", "10min"
};

// --- 4. 声明本页面的函数 ---
static void Page_Auto_Off_Enter(Page_Base* page);
static void Page_Auto_Off_Loop(Page_Base* page);
static void Page_Auto_Off_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset);
static void Page_Auto_Off_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event);

// --- 5. 定义页面全局实例 ---
Page_Base g_page_auto_off = {
    .enter = Page_Auto_Off_Enter,
    .exit = NULL,
    .loop = Page_Auto_Off_Loop,
    .draw = Page_Auto_Off_Draw,
    .action = Page_Auto_Off_Action,
    .page_name = "Auto-Off",
    .refresh_rate_ms = 30,
    .last_refresh_time = 0
};

// --- 6. 函数具体实现 ---

static void Page_Auto_Off_Enter(Page_Base* page) {
    Page_Auto_Off_Data_t* data = &g_page_auto_off_data;
    data->state = AUTO_OFF_STATE_IDLE;
    data->selected_index = g_app_settings.auto_off;

    // 计算初始可视区域
    if (data->selected_index >= VISIBLE_ITEMS) {
        data->viewport_top_index = data->selected_index - VISIBLE_ITEMS + 1;
    } else {
        data->viewport_top_index = 0;
    }
    
    // 计算高亮框的初始Y坐标
    int16_t highlight_y = LIST_TOP_Y + (data->selected_index - data->viewport_top_index) * AUTO_OFF_ITEM_HEIGHT;
    data->anim_current_y = highlight_y;
    data->anim_target_y = highlight_y;
    data->anim_start_y = highlight_y;
}

static void Page_Auto_Off_Loop(Page_Base* page) {
    Page_Auto_Off_Data_t* data = &g_page_auto_off_data;

    if (data->state == AUTO_OFF_STATE_SHOW_MSG) {
        if (HAL_GetTick() - data->msg_start_time >= 1000) {
            data->state = AUTO_OFF_STATE_IDLE;
            Go_Back_Page();
        }
        return;
    }

    if (data->state == AUTO_OFF_STATE_IDLE) {
        return;
    }

    uint32_t elapsed = HAL_GetTick() - data->anim_start_time;
    if (elapsed >= data->anim_duration) {
        data->anim_current_y = data->anim_target_y;
        data->state = AUTO_OFF_STATE_IDLE;
    } else {
        float progress = (float)elapsed / data->anim_duration;
        data->anim_current_y = data->anim_start_y + (data->anim_target_y - data->anim_start_y) * progress;
    }
}

static void Page_Auto_Off_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset) {
    Page_Auto_Off_Data_t* data = &g_page_auto_off_data;

    // --- 核心修改：统一计算逻辑 ---
    // 1. 计算列表的Y偏移。它总是等于 viewport_top_index 对应的偏移量。
    //    在列表滚动动画中，这个 viewport_top_index 会在动画开始时就更新到目标值。
    int16_t list_y_offset = -data->viewport_top_index * AUTO_OFF_ITEM_HEIGHT;

    // 2. 高亮框的Y坐标，就是我们的动画变量 anim_current_y。
    //    这个值代表高亮框在屏幕上的绝对位置。
    int16_t highlight_y = (int16_t)data->anim_current_y;

    // --- 绘制菜单项 (此部分逻辑不变) ---
    u8g2_SetFont(u8g2, MENU_FONT);
    u8g2_SetDrawColor(u8g2, 1);
    for (int i = 0; i < AUTO_OFF_ITEM_COUNT; i++) {
        // 计算每个菜单项的绝对Y坐标（相对于屏幕顶部）
        int16_t item_abs_y = LIST_TOP_Y + (i * AUTO_OFF_ITEM_HEIGHT) + list_y_offset;
        // 在垂直方向上裁剪，只绘制屏幕内的项
        if (item_abs_y + AUTO_OFF_ITEM_HEIGHT > 0 && item_abs_y < u8g2_GetDisplayHeight(u8g2)) {
            u8g2_DrawStr(u8g2, 15 + x_offset, item_abs_y + 12 + y_offset, menu_items[i]);
        }
    }

    // --- 绘制反色高亮框 (此部分逻辑不变，但现在数据源正确了) ---
    int16_t clip_x0 = AUTO_OFF_LEFT_X + x_offset;
    int16_t clip_y0 = highlight_y + y_offset;
    u8g2_SetClipWindow(u8g2, clip_x0, clip_y0, clip_x0 + AUTO_OFF_WIDTH, clip_y0 + AUTO_OFF_ITEM_HEIGHT);
    u8g2_SetDrawColor(u8g2, 1); 
    u8g2_DrawBox(u8g2, clip_x0, clip_y0, AUTO_OFF_WIDTH, AUTO_OFF_ITEM_HEIGHT);
    u8g2_SetDrawColor(u8g2, 0);
    for (int i = 0; i < AUTO_OFF_ITEM_COUNT; i++) {
        int16_t item_abs_y = LIST_TOP_Y + (i * AUTO_OFF_ITEM_HEIGHT) + list_y_offset;
        // 同样进行裁剪
        if (item_abs_y + AUTO_OFF_ITEM_HEIGHT > 0 && item_abs_y < u8g2_GetDisplayHeight(u8g2)) {
            u8g2_DrawStr(u8g2, 15 + x_offset, item_abs_y + 12 + y_offset, menu_items[i]);
        }
    }
    u8g2_SetMaxClipWindow(u8g2);
    u8g2_SetDrawColor(u8g2, 1);
    // 绘制保存反馈信息
    if (data->state == AUTO_OFF_STATE_SHOW_MSG) {
        u8g2_SetFont(u8g2, PROMPT_FONT);
        uint16_t msg_w = u8g2_GetStrWidth(u8g2, data->msg_text);
        uint16_t box_w = msg_w + 10;
        uint16_t box_h = 16;
        uint16_t box_x = (u8g2_GetDisplayWidth(u8g2) - box_w) / 2;
        uint16_t box_y = (u8g2_GetDisplayHeight(u8g2) - box_h) / 2;
        u8g2_SetDrawColor(u8g2, 0);
        u8g2_DrawBox(u8g2, box_x, box_y, box_w, box_h);
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawFrame(u8g2, box_x, box_y, box_w, box_h);
        u8g2_DrawStr(u8g2, box_x + 5, box_y + 12, data->msg_text);
    }
}

static void Page_Auto_Off_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event) {
    Page_Auto_Off_Data_t* data = &g_page_auto_off_data;

    if (data->state != AUTO_OFF_STATE_IDLE && data->state != AUTO_OFF_STATE_SHOW_MSG) {
        return;
    }
    if (data->state == AUTO_OFF_STATE_SHOW_MSG && event->event != INPUT_EVENT_BACK_PRESSED) {
        return;
    }

    switch (event->event) {
        case INPUT_EVENT_ENCODER: {
            int8_t old_index = data->selected_index;
            data->selected_index += event->value;

            // --- 核心修改1: 修正循环逻辑 ---
            if (data->selected_index >= AUTO_OFF_ITEM_COUNT) {
                data->selected_index = data->selected_index % AUTO_OFF_ITEM_COUNT;
            } else if (data->selected_index < 0) {
                data->selected_index = (data->selected_index % AUTO_OFF_ITEM_COUNT + AUTO_OFF_ITEM_COUNT) % AUTO_OFF_ITEM_COUNT;
            }

            if (old_index == data->selected_index) break;

            // --- 核心修改2: 修正 viewport 计算逻辑 ---
            int8_t old_viewport_top = data->viewport_top_index;
            // 判断新选中项是否需要滚动 viewport
            if (data->selected_index < data->viewport_top_index) {
                // 向上滚动，新选中项成为 viewport 顶部
                data->viewport_top_index = data->selected_index;
            } else if (data->selected_index >= (data->viewport_top_index + VISIBLE_ITEMS)) {
                // 向下滚动，新选中项成为 viewport 底部
                data->viewport_top_index = data->selected_index - VISIBLE_ITEMS + 1;
            }
            
            // --- 核心修改3: 统一启动动画 ---
            data->state = AUTO_OFF_STATE_ANIMATING_HIGHLIGHT; // 状态统一，因为我们只驱动高亮框
            data->anim_start_y = data->anim_current_y;
            // 目标Y值总是根据新的 viewport 和新的 selected_index 计算
            data->anim_target_y = LIST_TOP_Y + (data->selected_index - data->viewport_top_index) * AUTO_OFF_ITEM_HEIGHT;
            
            // 如果 viewport 变化了，意味着列表也需要“滚动”
            // 但我们的绘制逻辑会自动处理，我们只需要让动画时间长一点，看起来像列表滚动
            if (old_viewport_top != data->viewport_top_index) {
                data->anim_duration = 200; // 列表滚动动画时间长一点
            } else {
                data->anim_duration = 120; // 高亮框移动动画时间短一点
            }

            data->anim_start_time = HAL_GetTick();
            break;
        }
        case INPUT_EVENT_COMFIRM_PRESSED:
            g_app_settings.auto_off = data->selected_index;
            if (app_settings_save(&g_app_settings)) {
                data->msg_text = "Settings Saved!";
            } else {
                data->msg_text = "Save Failed!";
            }
            data->state = AUTO_OFF_STATE_SHOW_MSG;
            data->msg_start_time = HAL_GetTick();
            break;

        case INPUT_EVENT_BACK_PRESSED:
            Go_Back_Page();
            break;
        
        default:
            break;
    }
}