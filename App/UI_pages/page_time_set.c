/**
 * @file      page_time_set.c
 * @brief     时间与日期设置子菜单页面
 * @details   本文件定义了“时间与日期”设置的子菜单，包含“Date”, "Time", "DST"选项，并实现了带动画的菜单交互。
 * @author    SandOcean
 * @date      2025-09-17
 * @version   1.0
 * @copyright Copyright (c) 2025 SandOcean
 */

#include "app_display.h"
#include "input.h"

/* Private defines -----------------------------------------------------------*/
#define TIME_SET_ITEM_COUNT 3       ///< 菜单项数量
#define TIME_SET_ITEM_HEIGHT 16     ///< 每个菜单项的像素高度
#define TIME_SET_TOP_Y 8            ///< 菜单列表顶部的Y坐标
#define TIME_SET_LEFT_X 5           ///< 菜单列表左侧的X坐标
#define TIME_SET_WIDTH 118          ///< 菜单列表的像素宽度

/* Private variables ---------------------------------------------------------*/
///< 菜单项文本数组
static const char* menu_items[TIME_SET_ITEM_COUNT] = {
    "Date",
    "Time",
    "DST" // Daylight Saving Time
};

/**
 * @brief 菜单动画状态枚举
 */
typedef enum {
    TIME_SET_STATE_IDLE,        ///< 空闲状态
    TIME_SET_STATE_ANIMATING    ///< 动画播放中状态
} Time_Set_State_e;

/**
 * @brief 时间设置子菜单页面的私有数据结构体
 */
typedef struct {
    int8_t selected_index;      ///< 当前选中的菜单项索引
    Time_Set_State_e state;     ///< 菜单的动画状态
    float anim_current_y;       ///< 高亮框当前的Y坐标 (用于动画插值)
    int16_t anim_start_y;       ///< 高亮框动画的起始Y坐标
    int16_t anim_target_y;      ///< 高亮框动画的目标Y坐标
    uint32_t anim_start_time;   ///< 动画开始的系统时间
    uint32_t anim_duration;     ///< 动画持续时间 (ms)
} Page_Time_Set_Data_t;

static Page_Time_Set_Data_t g_page_time_set_data; ///< 时间设置子菜单页面的数据实例

/* Private function prototypes -----------------------------------------------*/
static void Page_Time_Set_Enter(Page_Base* page);
static void Page_Time_Set_Loop(Page_Base* page);
static void Page_Time_Set_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset);
static void Page_Time_Set_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event);

/* Public variables ----------------------------------------------------------*/
/**
 * @brief 时间设置子菜单页面的全局实例
 */
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

/* Function implementations --------------------------------------------------*/

/**
 * @brief 页面进入函数
 * @param page 指向页面基类的指针
 */
static void Page_Time_Set_Enter(Page_Base* page) {
    Page_Time_Set_Data_t* data = &g_page_time_set_data;
    data->state = TIME_SET_STATE_IDLE;
    data->selected_index = 0;

    int16_t initial_y = TIME_SET_TOP_Y + (data->selected_index * TIME_SET_ITEM_HEIGHT);
    data->anim_current_y = initial_y;
    data->anim_target_y = initial_y;
    data->anim_start_y = initial_y;
}

/**
 * @brief 页面循环逻辑函数 (处理动画)
 * @param page 指向页面基类的指针
 */
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

/**
 * @brief 页面绘制函数
 * @param page 指向页面基类的指针
 * @param u8g2 指向u8g2实例的指针
 * @param x_offset 屏幕的X方向偏移
 * @param y_offset 屏幕的Y方向偏移
 */
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

/**
 * @brief 页面输入事件处理函数
 * @param page 指向页面基类的指针
 * @param u8g2 指向u8g2实例的指针
 * @param event 指向输入事件数据的指针
 */
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