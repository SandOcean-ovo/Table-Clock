/**
 * @file page_time_set.c
 * @brief 时间与日期设置子菜单页面
 * @details 本文件定义了“时间与日期”设置的子菜单，包含“Date”, "Time", "DST"选项。
 * @author SandOcean
 * @date 2025-09-17
 * @version 1.0
 */

#include "app_display.h"
#include "input.h"

// --- 1. 页面私有定义 ---
#define TIME_SET_ITEM_COUNT 3
#define TIME_SET_ITEM_HEIGHT 16
#define TIME_SET_TOP_Y 8
#define TIME_SET_LEFT_X 5
#define TIME_SET_WIDTH 118

// 菜单项的字符串
static const char* menu_items[TIME_SET_ITEM_COUNT] = {
    "Date",
    "Time",
    "DST" // Daylight Saving Time
};

// 菜单动画状态
typedef enum {
    TIME_SET_STATE_IDLE,
    TIME_SET_STATE_ANIMATING
} Time_Set_State_e;

// --- 2. 定义页面私有数据结构体 ---
typedef struct {
    int8_t selected_index;
    Time_Set_State_e state;
    float anim_current_y;
    int16_t anim_start_y;
    int16_t anim_target_y;
    uint32_t anim_start_time;
    uint32_t anim_duration;
} Page_Time_Set_Data_t;

// --- 3. 声明并初始化页面私有数据 ---
static Page_Time_Set_Data_t g_page_time_set_data;

// --- 4. 声明本页面的函数 ---
static void Page_Time_Set_Enter(Page_Base* page);
static void Page_Time_Set_Loop(Page_Base* page);
static void Page_Time_Set_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset);
static void Page_Time_Set_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event);

// --- 5. 定义页面全局实例 ---
Page_Base g_page_time_set = {
    .enter = Page_Time_Set_Enter,
    .exit = NULL,
    .loop = Page_Time_Set_Loop,
    .draw = Page_Time_Set_Draw,
    .action = Page_Time_Set_Action,
    .page_name = "TimeSet",
    .refresh_rate_ms = 16, // ~60FPS
    .last_refresh_time = 0
};

// --- 6. 函数具体实现 ---

static void Page_Time_Set_Enter(Page_Base* page) {
    Page_Time_Set_Data_t* data = &g_page_time_set_data;
    data->state = TIME_SET_STATE_IDLE;
    data->selected_index = 0;

    int16_t initial_y = TIME_SET_TOP_Y + (data->selected_index * TIME_SET_ITEM_HEIGHT);
    data->anim_current_y = initial_y;
    data->anim_target_y = initial_y;
    data->anim_start_y = initial_y;
}

static void Page_Time_Set_Loop(Page_Base* page) {
    Page_Time_Set_Data_t* data = &g_page_time_set_data;

    if (data->state != TIME_SET_STATE_ANIMATING) {
        return;
    }

    uint32_t elapsed = HAL_GetTick() - data->anim_start_time;
    if (elapsed >= data->anim_duration) {
        data->anim_current_y = data->anim_target_y;
        data->state = TIME_SET_STATE_IDLE;
    } else {
        float progress = (float)elapsed / data->anim_duration;
        data->anim_current_y = data->anim_start_y + (data->anim_target_y - data->anim_start_y) * progress;
    }
}

static void Page_Time_Set_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset) {
    Page_Time_Set_Data_t* data = &g_page_time_set_data;

    // 绘制菜单项
    u8g2_SetFont(u8g2, MENU_FONT);
    u8g2_SetDrawColor(u8g2, 1);
    for (int i = 0; i < TIME_SET_ITEM_COUNT; i++) {
        u8g2_DrawStr(u8g2, 15 + x_offset, (i * TIME_SET_ITEM_HEIGHT) + TIME_SET_TOP_Y + 12 + y_offset, menu_items[i]);
    }

    // 绘制反色高亮框
    int16_t clip_x0 = TIME_SET_LEFT_X + x_offset;
    int16_t clip_y0 = (int16_t)data->anim_current_y + y_offset;
    int16_t clip_x1 = clip_x0 + TIME_SET_WIDTH;
    int16_t clip_y1 = clip_y0 + TIME_SET_ITEM_HEIGHT;
    u8g2_SetClipWindow(u8g2, clip_x0, clip_y0, clip_x1, clip_y1);
    u8g2_SetDrawColor(u8g2, 1); 
    u8g2_DrawBox(u8g2, clip_x0, clip_y0, TIME_SET_WIDTH, TIME_SET_ITEM_HEIGHT);
    u8g2_SetDrawColor(u8g2, 0);
    for (int i = 0; i < TIME_SET_ITEM_COUNT; i++) {
        u8g2_DrawStr(u8g2, 15 + x_offset, (i * TIME_SET_ITEM_HEIGHT) + TIME_SET_TOP_Y + 12 + y_offset, menu_items[i]);
    }
    u8g2_SetMaxClipWindow(u8g2);
    u8g2_SetDrawColor(u8g2, 1);
}

static void Page_Time_Set_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event) {
    Page_Time_Set_Data_t* data = &g_page_time_set_data;

    if (data->state == TIME_SET_STATE_ANIMATING) {
        return;
    }

    switch (event->event) {
        case INPUT_EVENT_ENCODER: {
            int8_t old_index = data->selected_index;
            data->selected_index += event->value;
            if (data->selected_index >= TIME_SET_ITEM_COUNT) data->selected_index = 0;
            if (data->selected_index < 0) data->selected_index = TIME_SET_ITEM_COUNT - 1;

            if (old_index != data->selected_index) {
                data->state = TIME_SET_STATE_ANIMATING;
                data->anim_start_time = HAL_GetTick();
                data->anim_duration = 150;
                data->anim_start_y = data->anim_current_y;
                data->anim_target_y = TIME_SET_TOP_Y + (data->selected_index * TIME_SET_ITEM_HEIGHT);
            }
            break;
        }
        case INPUT_EVENT_COMFIRM_PRESSED:
            switch (data->selected_index) {
                case 0: // Date
                    Switch_Page(&g_page_time_date); 
                    break;
                case 1: // Time
                    Switch_Page(&g_page_time_time); 
                    break;
                case 2: // DST
                    Switch_Page(&g_page_time_dst); 
                    break;
            }
            break;

        case INPUT_EVENT_BACK_PRESSED:
            Go_Back_Page();
            break;
        
        default:
            break;
    }
}