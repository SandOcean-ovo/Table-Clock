/**
 * @file      page_time_dst.c
 * @brief     夏令时设置页面
 * @details   本文件定义了“夏令时”设置菜单，用于开启或关闭夏令时功能，并实现了带动画的菜单交互。
 * @version   1.0
 * @date      2025-09-17
 * @author    SandOcean
 * @copyright Copyright (c) 2025 SandOcean
 */

#include "app_display.h"
#include "input.h"
#include "app_config.h"
#include "app_settings.h"

/* Private defines -----------------------------------------------------------*/
#define DST_ITEM_COUNT 2   ///< 菜单项数量
#define DST_ITEM_HEIGHT 16 ///< 每个菜单项的像素高度
#define DST_TOP_Y 16       ///< 菜单列表顶部的Y坐标
#define DST_LEFT_X 5       ///< 菜单列表左侧的X坐标
#define DST_WIDTH 118      ///< 菜单列表的像素宽度

/* Private variables ---------------------------------------------------------*/
///< 菜单项文本数组
static const char *menu_items[DST_ITEM_COUNT] = {
    "Off",
    "On"};

/**
 * @brief 菜单状态枚举
 */
typedef enum
{
    DST_STATE_IDLE,      ///< 空闲状态
    DST_STATE_ANIMATING, ///< 动画播放中状态
    DST_STATE_SHOW_MSG   ///< 显示反馈信息状态
} Dst_State_e;

/**
 * @brief 夏令时设置页面的私有数据结构体
 */
typedef struct
{
    Page_Base base;           ///< 必须包含基类作为第一个成员
    int8_t selected_index;    ///< 当前选中的菜单项索引
    Dst_State_e state;        ///< 菜单的动画状态
    float anim_current_y;     ///< 高亮框当前的Y坐标 (用于动画插值)
    int16_t anim_start_y;     ///< 高亮框动画的起始Y坐标
    int16_t anim_target_y;    ///< 高亮框动画的目标Y坐标
    uint32_t anim_start_time; ///< 动画开始的系统时间
    uint32_t anim_duration;   ///< 动画持续时间 (ms)
    uint32_t msg_start_time;  ///< 反馈信息显示的开始时间戳
    const char *msg_text;     ///< 指向要显示的反馈信息字符串
} Page_Dst_Data_t;

static Page_Dst_Data_t g_page_dst_data; ///< 夏令时设置页面的数据实例

/* Private function prototypes -----------------------------------------------*/
static void Page_Dst_Enter(Page_Base *page);
static void Page_Dst_Loop(Page_Base *page);
static void Page_Dst_Draw(Page_Base *page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset);
static void Page_Dst_Action(Page_Base *page, u8g2_t *u8g2, const Input_Event_Data_t *event);

/* Public variables ----------------------------------------------------------*/
/**
 * @brief 夏令时设置页面的全局实例
 */
Page_Base g_page_time_dst = {
    .enter = Page_Dst_Enter,
    .exit = NULL,
    .loop = Page_Dst_Loop,
    .draw = Page_Dst_Draw,
    .action = Page_Dst_Action,
    .page_name = "DST",
    .refresh_rate_ms = 30, // ~33FPS
    .last_refresh_time = 0};

/* Function implementations --------------------------------------------------*/

/**
 * @brief 页面进入函数
 * @param[in] page 指向页面基类的指针
 * @return 无
 */
static void Page_Dst_Enter(Page_Base *page)
{
    Page_Dst_Data_t *data = &g_page_dst_data;
    data->state = DST_STATE_IDLE;

    // 从全局配置中读取当前夏令时设置
    data->selected_index = g_app_settings.dst_enabled;

    // 初始化高亮框坐标
    int16_t initial_y = DST_TOP_Y + (data->selected_index * DST_ITEM_HEIGHT);
    data->anim_current_y = initial_y;
    data->anim_target_y = initial_y;
    data->anim_start_y = initial_y;
}

/**
 * @brief 页面循环逻辑函数 (处理动画和消息显示)
 * @param[in] page 指向页面基类的指针
 * @return 无
 */
static void Page_Dst_Loop(Page_Base *page)
{
    Page_Dst_Data_t *data = &g_page_dst_data;

    if (data->state == DST_STATE_SHOW_MSG)
    {
        if (HAL_GetTick() - data->msg_start_time >= 1000)
        {
            data->state = DST_STATE_IDLE; // 恢复状态
            Go_Back_Page();
        }
        return;
    }

    if (data->state != DST_STATE_ANIMATING)
    {
        return;
    }

    uint32_t elapsed = HAL_GetTick() - data->anim_start_time;
    if (elapsed >= data->anim_duration)
    {
        data->anim_current_y = data->anim_target_y;
        data->state = DST_STATE_IDLE;
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
static void Page_Dst_Draw(Page_Base *page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset)
{
    Page_Dst_Data_t *data = &g_page_dst_data;

    // 绘制菜单项
    u8g2_SetFont(u8g2, MENU_FONT);
    u8g2_SetDrawColor(u8g2, 1);
    for (int i = 0; i < DST_ITEM_COUNT; i++)
    {
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
    for (int i = 0; i < DST_ITEM_COUNT; i++)
    {
        u8g2_DrawStr(u8g2, 15 + x_offset, (i * DST_ITEM_HEIGHT) + DST_TOP_Y + 12 + y_offset, menu_items[i]);
    }
    u8g2_SetMaxClipWindow(u8g2);
    u8g2_SetDrawColor(u8g2, 1);

    // 绘制保存反馈信息
    if (data->state == DST_STATE_SHOW_MSG)
    {
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

/**
 * @brief 页面输入事件处理函数
 * @param[in] page 指向页面基类的指针
 * @param[in] u8g2 指向u8g2实例的指针
 * @param[in] event 指向输入事件数据的指针
 * @return 无
 */
static void Page_Dst_Action(Page_Base *page, u8g2_t *u8g2, const Input_Event_Data_t *event)
{
    Page_Dst_Data_t *data = &g_page_dst_data;

    if (data->state == DST_STATE_SHOW_MSG || data->state == DST_STATE_ANIMATING)
    {
        return;
    }

    switch (event->event)
    {
    case INPUT_EVENT_ENCODER:
    {
        int8_t old_index = data->selected_index;
        data->selected_index += event->value;
        if (data->selected_index >= DST_ITEM_COUNT)
            data->selected_index = 0;
        if (data->selected_index < 0)
            data->selected_index = DST_ITEM_COUNT - 1;

        if (old_index != data->selected_index)
        {
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
        if (app_settings_save(&g_app_settings))
        {
            data->msg_text = "Settings Saved!";
        }
        else
        {
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