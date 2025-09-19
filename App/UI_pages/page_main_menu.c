/**
 * @file      page_main_menu.c
 * @brief     主菜单页面实现文件
 * @details   本文件定义了主菜单页面的行为，并实现了带动画的菜单交互。
 * @author    SandOcean
 * @date      2025-09-15
 * @version   1.0
 * @copyright Copyright (c) 2025 SandOcean
 */

#include "app_display.h"
#include "DS3231.h"
#include "AHT20.h"
#include "input.h"

/* Private defines -----------------------------------------------------------*/
#define MENU_ITEM_COUNT 3   ///< 菜单项数量
#define MENU_ITEM_HEIGHT 16 ///< 每个菜单项的像素高度
#define MENU_TOP_Y 8        ///< 菜单列表顶部的Y坐标
#define MENU_LEFT_X 2       ///< 菜单列表左侧的X坐标
#define MENU_WIDTH 118      ///< 菜单列表的像素宽度

/* Private variables ---------------------------------------------------------*/
/**
 * @brief 菜单动画状态枚举
 */
typedef enum
{
    MENU_STATE_IDLE,     ///< 静止状态
    MENU_STATE_ANIMATING ///< 动画播放中状态
} Menu_State_e;

/**
 * @brief 主菜单页面的私有数据结构体
 */
typedef struct
{
    Page_Base base;           ///< 必须包含基类作为第一个成员
    int8_t selected_index;    ///< 当前选择的菜单项索引
    Menu_State_e state;       ///< 菜单自身的动画状态
    float anim_current_y;     ///< 高亮框当前的Y坐标 (使用float以实现平滑移动)
    int16_t anim_start_y;     ///< 高亮框动画的起始Y坐标
    int16_t anim_target_y;    ///< 高亮框的目标Y坐标
    uint32_t anim_start_time; ///< 动画开始时间
    uint32_t anim_duration;   ///< 动画总时长
} Page_main_menu_Data;

static Page_main_menu_Data g_page_main_menu_data; ///< 主菜单页面的数据实例

/* Private function prototypes -----------------------------------------------*/
static void Page_main_menu_Enter(Page_Base *page);
static void Page_main_menu_Loop(Page_Base *page);
static void Page_main_menu_Draw(Page_Base *page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset);
static void Page_main_menu_Action(Page_Base *page, u8g2_t *u8g2, const Input_Event_Data_t *event);

/* Public variables ----------------------------------------------------------*/
/**
 * @brief 主菜单页面的全局实例
 */
Page_Base g_page_main_menu = {
    .enter = Page_main_menu_Enter,
    .exit = NULL,
    .loop = Page_main_menu_Loop,
    .draw = Page_main_menu_Draw,
    .action = Page_main_menu_Action,
    .page_name = "main_menu",
    .parent_page = &g_page_main,
    .refresh_rate_ms = 30, // 提高刷新率以保证动画流畅 (~33FPS)
    .last_refresh_time = 0};

/* Function implementations --------------------------------------------------*/
/**
 * @brief 页面进入函数
 * @param[in] page 指向页面基类的指针
 * @return 无
 */
static void Page_main_menu_Enter(Page_Base *page)
{
    Page_main_menu_Data *data = &g_page_main_menu_data;
    data->state = MENU_STATE_IDLE;
    data->selected_index = 0;
    data->anim_current_y = MENU_TOP_Y + (data->selected_index * (MENU_ITEM_HEIGHT));
    data->anim_start_y = data->anim_current_y; // 初始化起始Y坐标
}

/**
 * @brief 页面循环逻辑函数 (处理动画)
 * @param[in] page 指向页面基类的指针
 * @return 无
 */
static void Page_main_menu_Loop(Page_Base *page)
{
    Page_main_menu_Data *data = &g_page_main_menu_data;

    if (data->state != MENU_STATE_ANIMATING)
    {
        return;
    }

    uint32_t elapsed = HAL_GetTick() - data->anim_start_time;

    if (elapsed >= data->anim_duration)
    {
        data->anim_current_y = data->anim_target_y;
        data->state = MENU_STATE_IDLE;
    }
    else
    {
        float progress = (float)elapsed / data->anim_duration;
        data->anim_current_y = data->anim_start_y + (data->anim_target_y - data->anim_start_y) * progress;
    }
}

/**
 * @brief 页面绘制函数
 * @param[in] page 指向页面基类的指针
 * @param[in] u8g2 指向u8g2实例的指针
 * @param[in] x_offset 屏幕的X方向偏移
 * @param[in] y_offset 屏幕的Y方向偏移
 * @return 无
 */
static void Page_main_menu_Draw(Page_Base *page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset)
{
    Page_main_menu_Data *data = &g_page_main_menu_data;
    const char *menu_items[] = {"Display", "Time Set", "Info"};

    u8g2_SetFont(u8g2, MENU_FONT);

    u8g2_SetDrawColor(u8g2, 1);
    for (int i = 0; i < MENU_ITEM_COUNT; i++)
    {
        u8g2_DrawStr(u8g2, 15 + x_offset, (i * MENU_ITEM_HEIGHT) + MENU_TOP_Y + 12 + y_offset, menu_items[i]);
    }

    int16_t clip_x0 = MENU_LEFT_X + x_offset;
    int16_t clip_y0 = (int16_t)data->anim_current_y + y_offset;
    int16_t clip_x1 = clip_x0 + MENU_WIDTH;
    int16_t clip_y1 = clip_y0 + MENU_ITEM_HEIGHT;

    u8g2_SetClipWindow(u8g2, clip_x0, clip_y0, clip_x1, clip_y1);

    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawBox(u8g2, clip_x0, clip_y0, MENU_WIDTH, MENU_ITEM_HEIGHT);

    u8g2_SetDrawColor(u8g2, 0);
    for (int i = 0; i < MENU_ITEM_COUNT; i++)
    {
        u8g2_DrawStr(u8g2, 15 + x_offset, (i * MENU_ITEM_HEIGHT) + MENU_TOP_Y + 12 + y_offset, menu_items[i]);
    }

    u8g2_SetMaxClipWindow(u8g2);

    u8g2_SetDrawColor(u8g2, 1);
}

/**
 * @brief 页面输入事件处理函数
 * @param[in] page 指向页面基类的指针
 * @param[in] u8g2 指向u8g2实例的指针
 * @param[in] event 指向输入事件数据的指针
 * @return 无
 */
static void Page_main_menu_Action(Page_Base *page, u8g2_t *u8g2, const Input_Event_Data_t *event)
{
    Page_main_menu_Data *data = &g_page_main_menu_data;

    if (data->state == MENU_STATE_ANIMATING)
    {
        return;
    }

    switch (event->event)
    {
    case INPUT_EVENT_ENCODER:
    {
        int8_t old_index = data->selected_index;
        data->selected_index += event->value;

        if (data->selected_index >= MENU_ITEM_COUNT)
        {
            data->selected_index = 0;
        }
        if (data->selected_index < 0)
        {
            data->selected_index = MENU_ITEM_COUNT - 1;
        }

        if (old_index != data->selected_index)
        {
            data->state = MENU_STATE_ANIMATING;
            data->anim_start_time = HAL_GetTick();
            data->anim_duration = 150;
            data->anim_start_y = data->anim_current_y; // 保存动画起点
            data->anim_target_y = MENU_TOP_Y + (data->selected_index * (MENU_ITEM_HEIGHT));
        }
        break;
    }
    case INPUT_EVENT_COMFIRM_PRESSED:
        if (data->selected_index == 0)
            Switch_Page(&g_page_display);
        if (data->selected_index == 1)
            Switch_Page(&g_page_time_set);
        if (data->selected_index == 2)
            Switch_Page(&g_page_info);
        break;
    case INPUT_EVENT_BACK_PRESSED:
        Go_Back_Page();
        break;
    }
}