/**
 * @file page_time_dst.c
 * @brief 夏令时设置页面
 * @details 本文件定义了“夏令时”设置菜单，用于开启或关闭夏令时功能。
 * @author SandOcean
 * @date 2025-09-17
 * @version 1.0
 */

#include "app_display.h"
#include "input.h"
#include "app_config.h"
#include "app_settings.h"

// --- 1. 页面私有定义 ---
#define DST_ITEM_COUNT 2
#define DST_ITEM_HEIGHT 16
#define DST_TOP_Y 16
#define DST_LEFT_X 5
#define DST_WIDTH 118

// 菜单项的字符串
static const char* menu_items[DST_ITEM_COUNT] = {
    "Off",
    "On"
};

// 菜单状态
typedef enum {
    DST_STATE_IDLE,
    DST_STATE_ANIMATING,
    DST_STATE_SHOW_MSG
} Dst_State_e;

// --- 2. 定义页面私有数据结构体 ---
typedef struct {
    Page_Base base; // 必须包含基类作为第一个成员
    // 私有数据
    int8_t selected_index;
    Dst_State_e state;
    float anim_current_y;
    int16_t anim_start_y;
    int16_t anim_target_y;
    uint32_t anim_start_time;
    uint32_t anim_duration;
    uint32_t msg_start_time;
    const char* msg_text;
} Page_Dst_Data_t;

// --- 3. 声明并初始化页面私有数据 ---
static Page_Dst_Data_t g_page_dst_data;

// --- 4. 声明本页面的函数 ---
static void Page_Dst_Enter(Page_Base* page);
static void Page_Dst_Loop(Page_Base* page);
static void Page_Dst_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset);
static void Page_Dst_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event);

// --- 5. 定义页面全局实例 ---
Page_Base g_page_time_dst = {
    .enter = Page_Dst_Enter,
    .exit = NULL,
    .loop = Page_Dst_Loop,
    .draw = Page_Dst_Draw,
    .action = Page_Dst_Action,
    .page_name = "DST",
    .refresh_rate_ms = 30, // ~33FPS
    .last_refresh_time = 0
};

// --- 6. 函数具体实现 ---

static void Page_Dst_Enter(Page_Base* page) {
    Page_Dst_Data_t* data = &g_page_dst_data;
    data->state = DST_STATE_IDLE;

    // 从全局配置中读取当前夏令时设置
    data->selected_index = g_app_settings.dst_enabled;

    // 初始化高亮框坐标
    int16_t initial_y = DST_TOP_Y + (data->selected_index * DST_ITEM_HEIGHT);
    data->anim_current_y = initial_y;
    data->anim_target_y = initial_y;
    data->anim_start_y = initial_y;
}

static void Page_Dst_Loop(Page_Base* page) {
    Page_Dst_Data_t* data = &g_page_dst_data;

    if (data->state == DST_STATE_SHOW_MSG) {
        if (HAL_GetTick() - data->msg_start_time >= 1000) {
            data->state = DST_STATE_IDLE; // 恢复状态
            Go_Back_Page();
        }
        return;
    }

    if (data->state != DST_STATE_ANIMATING) {
        return;
    }

    uint32_t elapsed = HAL_GetTick() - data->anim_start_time;
    if (elapsed >= data->anim_duration) {
        data->anim_current_y = data->anim_target_y;
        data->state = DST_STATE_IDLE;
    } else {
        float progress = (float)elapsed / data->anim_duration;
        data->anim_current_y = data->anim_start_y + (data->anim_target_y - data->anim_start_y) * progress;
    }
}

static void Page_Dst_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset) {
    Page_Dst_Data_t* data = &g_page_dst_data;

    // 绘制菜单项
    u8g2_SetFont(u8g2, MENU_FONT);
    u8g2_SetDrawColor(u8g2, 1);
    for (int i = 0; i < DST_ITEM_COUNT; i++) {
        u8g2_DrawStr(u8g2, 15 + x_offset, (i * DST_ITEM_HEIGHT) + DST_TOP_Y + 12 + y_offset, menu_items[i]);
    }

    // 绘制反色高亮框
    int16_t clip_x0 = DST_LEFT_X + x_offset;
    int16_t clip_y0 = (int16_t)data->anim_current_y + y_offset;
    int16_t clip_x1 = clip_x0 + DST_WIDTH;
    int16_t clip_y1 = clip_y0 + DST_ITEM_HEIGHT;
    u8g2_SetClipWindow(u8g2, clip_x0, clip_y0, clip_x1, clip_y1);
    u8g2_SetDrawColor(u8g2, 1); 
    u8g2_DrawBox(u8g2, clip_x0, clip_y0, DST_WIDTH, DST_ITEM_HEIGHT);
    u8g2_SetDrawColor(u8g2, 0);
    for (int i = 0; i < DST_ITEM_COUNT; i++) {
        u8g2_DrawStr(u8g2, 15 + x_offset, (i * DST_ITEM_HEIGHT) + DST_TOP_Y + 12 + y_offset, menu_items[i]);
    }
    u8g2_SetMaxClipWindow(u8g2);
    u8g2_SetDrawColor(u8g2, 1);

    // 绘制保存反馈信息
    if (data->state == DST_STATE_SHOW_MSG) {
        u8g2_SetFont(u8g2, PROMPT_FONT);
        uint16_t msg_w = u8g2_GetStrWidth(u8g2, data->msg_text);
        uint16_t box_w = msg_w + 10;
        uint16_t box_h = 16;
        uint16_t box_x = (u8g2_GetDisplayWidth(u8g2) - box_w) / 2;
        uint16_t box_y = (u8g2_GetDisplayHeight(u8g2) - box_h) / 2;

        // 绘制一个黑色背景框，覆盖下方内容
        u8g2_SetDrawColor(u8g2, 0);
        u8g2_DrawBox(u8g2, box_x, box_y, box_w, box_h);

        // 绘制白色边框
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawFrame(u8g2, box_x, box_y, box_w, box_h);

        // 绘制文字
        u8g2_DrawStr(u8g2, box_x + 5, box_y + 12, data->msg_text);
    }
}

static void Page_Dst_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event) {
    Page_Dst_Data_t* data = &g_page_dst_data;

    if (data->state == DST_STATE_SHOW_MSG || data->state == DST_STATE_ANIMATING) {
        return;
    }

    switch (event->event) {
        case INPUT_EVENT_ENCODER: {
            int8_t old_index = data->selected_index;
            data->selected_index += event->value;
            if (data->selected_index >= DST_ITEM_COUNT) data->selected_index = 0;
            if (data->selected_index < 0) data->selected_index = DST_ITEM_COUNT - 1;

            if (old_index != data->selected_index) {
                data->state = DST_STATE_ANIMATING;
                data->anim_start_time = HAL_GetTick();
                data->anim_duration = 150;
                data->anim_start_y = data->anim_current_y;
                data->anim_target_y = DST_TOP_Y + (data->selected_index * DST_ITEM_HEIGHT);
            }
            break;
        }
        case INPUT_EVENT_COMFIRM_PRESSED:
            g_app_settings.dst_enabled = data->selected_index;
            if (app_settings_save(&g_app_settings)) {
                data->msg_text = "Settings Saved!";
            } else {
                data->msg_text = "Save Failed!";
            }
            data->state = DST_STATE_SHOW_MSG;
            data->msg_start_time = HAL_GetTick();
            break;

        case INPUT_EVENT_BACK_PRESSED:
            Go_Back_Page();
            break;
        
        default:
            break;
    }
}