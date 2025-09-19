/**
 * @file      page_auto_off.c
 * @brief     自动熄屏设置页面
 * @details   本文件定义了“自动熄屏”设置菜单的UI和交互逻辑，
 *            实现了列表滚动和高亮框移动的平滑动画效果。
 * @author    SandOcean
 * @date      2025-09-17
 * @version   1.3
 * @copyright Copyright (c) 2025 SandOcean
 */

#include "app_display.h"
#include "input.h"
#include "app_config.h"
#include "app_settings.h"

/**
 * @defgroup PageAutoOff 自动熄屏设置页面
 * @{
 */

/* Private defines -----------------------------------------------------------*/
#define AUTO_OFF_ITEM_COUNT 5   ///< 菜单项总数
#define AUTO_OFF_ITEM_HEIGHT 16 ///< 每个菜单项的像素高度
#define AUTO_OFF_LEFT_X 5       ///< 列表区域左侧的X坐标
#define AUTO_OFF_WIDTH 118      ///< 列表区域的像素宽度
#define VISIBLE_ITEMS 4         ///< 屏幕上最多可见的菜单项数量
#define LIST_TOP_Y 0            ///< 列表区域的起始Y坐标

/**
 * @brief 页面动画状态枚举
 */
typedef enum
{
    AUTO_OFF_STATE_IDLE,                ///< 空闲状态，等待用户输入
    AUTO_OFF_STATE_ANIMATING_HIGHLIGHT, ///< 动画状态：仅移动高亮框
    AUTO_OFF_STATE_ANIMATING_LIST,      ///< 动画状态：滚动整个列表 (当前版本中已统一为高亮框动画)
    AUTO_OFF_STATE_SHOW_MSG             ///< 显示反馈信息状态
} Auto_Off_State_e;

/**
 * @brief 自动熄屏设置页面的私有数据结构体
 * @details 该结构体包含了页面运行所需的所有状态和数据
 */
typedef struct
{
    int8_t selected_index;     ///< 当前选中的菜单项绝对索引 (0 到 ITEM_COUNT-1)
    int8_t viewport_top_index; ///< 屏幕可视区域顶部对应的菜单项索引
    Auto_Off_State_e state;    ///< 当前页面的状态

    float anim_current_y;     ///< 动画插值计算出的当前Y坐标 (用于高亮框)
    int16_t anim_start_y;     ///< 动画起始Y坐标
    int16_t anim_target_y;    ///< 动画目标Y坐标
    uint32_t anim_start_time; ///< 动画开始的HAL Tick时间戳
    uint32_t anim_duration;   ///< 动画持续时间 (ms)

    uint32_t msg_start_time; ///< 反馈信息显示的开始时间戳
    const char *msg_text;    ///< 指向要显示的反馈信息字符串
} Page_Auto_Off_Data_t;

/* Private variables ---------------------------------------------------------*/
static Page_Auto_Off_Data_t g_page_auto_off_data; ///< 自动熄屏页面的数据实例

/**
 * @brief 菜单项文本数组
 */
static const char *menu_items[AUTO_OFF_ITEM_COUNT] = {
    "Never", "30s", "1min", "5min", "10min"};

/* Private function prototypes -----------------------------------------------*/
static void Page_Auto_Off_Enter(Page_Base *page);
static void Page_Auto_Off_Loop(Page_Base *page);
static void Page_Auto_Off_Draw(Page_Base *page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset);
static void Page_Auto_Off_Action(Page_Base *page, u8g2_t *u8g2, const Input_Event_Data_t *event);

/* Public variables ----------------------------------------------------------*/
/**
 * @brief 自动熄屏设置页面的全局实例
 * @details 该实例包含了进入、退出、循环、绘制和事件处理的函数指针
 */
Page_Base g_page_auto_off = {
    .enter = Page_Auto_Off_Enter,
    .exit = NULL,
    .loop = Page_Auto_Off_Loop,
    .draw = Page_Auto_Off_Draw,
    .action = Page_Auto_Off_Action,
    .page_name = "Auto-Off",
    .refresh_rate_ms = 30,
    .last_refresh_time = 0};

/* Function implementations --------------------------------------------------*/

/**
 * @brief  页面进入函数
 * @details 当切换到此页面时被调用，用于初始化页面数据和状态。
 * @param[in] page 指向页面基类的指针 (未使用)
 * @return 无
 */
static void Page_Auto_Off_Enter(Page_Base *page)
{
    Page_Auto_Off_Data_t *data = &g_page_auto_off_data;
    data->state = AUTO_OFF_STATE_IDLE;
    data->selected_index = g_app_settings.auto_off;

    // 根据当前选中的设置，计算初始可视区域的起始索引
    if (data->selected_index >= VISIBLE_ITEMS)
    {
        data->viewport_top_index = data->selected_index - VISIBLE_ITEMS + 1;
    }
    else
    {
        data->viewport_top_index = 0;
    }

    // 计算高亮框的初始Y坐标，并初始化动画参数
    int16_t highlight_y = LIST_TOP_Y + (data->selected_index - data->viewport_top_index) * AUTO_OFF_ITEM_HEIGHT;
    data->anim_current_y = highlight_y;
    data->anim_target_y = highlight_y;
    data->anim_start_y = highlight_y;
}

/**
 * @brief  页面循环函数
 * @details 在每次页面刷新时被调用，用于处理动画更新和状态切换。
 * @param[in] page 指向页面基类的指针 (未使用)
 * @return 无
 */
static void Page_Auto_Off_Loop(Page_Base *page)
{
    Page_Auto_Off_Data_t *data = &g_page_auto_off_data;

    // 处理 "Settings Saved!" 等反馈信息的显示超时
    if (data->state == AUTO_OFF_STATE_SHOW_MSG)
    {
        if (HAL_GetTick() - data->msg_start_time >= 1000)
        {
            data->state = AUTO_OFF_STATE_IDLE;
            Go_Back_Page(); // 显示1秒后自动返回上一页
        }
        return;
    }

    // 如果当前不是动画状态，则直接返回
    if (data->state == AUTO_OFF_STATE_IDLE)
    {
        return;
    }

    // 计算动画插值
    uint32_t elapsed = HAL_GetTick() - data->anim_start_time;
    if (elapsed >= data->anim_duration)
    {
        // 动画结束
        data->anim_current_y = data->anim_target_y;
        data->state = AUTO_OFF_STATE_IDLE;
    }
    else
    {
        // 动画进行中，使用线性插值计算当前Y坐标
        float progress = (float)elapsed / data->anim_duration;
        data->anim_current_y = data->anim_start_y + (data->anim_target_y - data->anim_start_y) * progress;
    }
}

/**
 * @brief  页面绘制函数
 * @details 负责在屏幕上绘制所有UI元素，包括菜单列表、高亮框和反馈信息。
 * @param[in] page 指向页面基类的指针 (未使用)
 * @param[in] u8g2 指向 u8g2 实例的指针
 * @param[in] x_offset 屏幕的X方向偏移 (未使用)
 * @param[in] y_offset 屏幕的Y方向偏移 (未使用)
 * @return 无
 */
static void Page_Auto_Off_Draw(Page_Base *page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset)
{
    Page_Auto_Off_Data_t *data = &g_page_auto_off_data;

    // 计算列表的Y偏移，实现列表滚动的视觉效果
    int16_t list_y_offset = -data->viewport_top_index * AUTO_OFF_ITEM_HEIGHT;

    // 高亮框的Y坐标直接由动画变量控制
    int16_t highlight_y = (int16_t)data->anim_current_y;

    // 绘制菜单项列表
    u8g2_SetFont(u8g2, MENU_FONT);
    u8g2_SetDrawColor(u8g2, 1);
    for (int i = 0; i < AUTO_OFF_ITEM_COUNT; i++)
    {
        int16_t item_abs_y = LIST_TOP_Y + (i * AUTO_OFF_ITEM_HEIGHT) + list_y_offset;
        // 裁剪，只绘制屏幕范围内的项
        if (item_abs_y + AUTO_OFF_ITEM_HEIGHT > 0 && item_abs_y < u8g2_GetDisplayHeight(u8g2))
        {
            u8g2_DrawStr(u8g2, 15 + x_offset, item_abs_y + 12 + y_offset, menu_items[i]);
        }
    }

    // 使用裁剪窗口(ClipWindow)和反色绘制高亮框效果
    int16_t clip_x0 = AUTO_OFF_LEFT_X + x_offset;
    int16_t clip_y0 = highlight_y + y_offset;
    u8g2_SetClipWindow(u8g2, clip_x0, clip_y0, clip_x0 + AUTO_OFF_WIDTH, clip_y0 + AUTO_OFF_ITEM_HEIGHT);
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawBox(u8g2, clip_x0, clip_y0, AUTO_OFF_WIDTH, AUTO_OFF_ITEM_HEIGHT);
    u8g2_SetDrawColor(u8g2, 0); // 设置为反色
    for (int i = 0; i < AUTO_OFF_ITEM_COUNT; i++)
    {
        int16_t item_abs_y = LIST_TOP_Y + (i * AUTO_OFF_ITEM_HEIGHT) + list_y_offset;
        if (item_abs_y + AUTO_OFF_ITEM_HEIGHT > 0 && item_abs_y < u8g2_GetDisplayHeight(u8g2))
        {
            u8g2_DrawStr(u8g2, 15 + x_offset, item_abs_y + 12 + y_offset, menu_items[i]);
        }
    }
    u8g2_SetMaxClipWindow(u8g2); // 恢复裁剪窗口到最大
    u8g2_SetDrawColor(u8g2, 1);  // 恢复绘制颜色

    // 绘制保存反馈信息弹窗
    if (data->state == AUTO_OFF_STATE_SHOW_MSG)
    {
        u8g2_SetFont(u8g2, PROMPT_FONT);
        uint16_t msg_w = u8g2_GetStrWidth(u8g2, data->msg_text);
        uint16_t box_w = msg_w + 10;
        uint16_t box_h = 16;
        uint16_t box_x = (u8g2_GetDisplayWidth(u8g2) - box_w) / 2;
        uint16_t box_y = (u8g2_GetDisplayHeight(u8g2) - box_h) / 2;
        u8g2_SetDrawColor(u8g2, 0); // 背景涂黑
        u8g2_DrawBox(u8g2, box_x, box_y, box_w, box_h);
        u8g2_SetDrawColor(u8g2, 1); // 边框和文字用白色
        u8g2_DrawFrame(u8g2, box_x, box_y, box_w, box_h);
        u8g2_DrawStr(u8g2, box_x + 5, box_y + 12, data->msg_text);
    }
}

/**
 * @brief  页面事件处理函数
 * @details 处理来自输入设备的事件，如旋钮旋转、按键按下等。
 * @param[in] page 指向页面基类的指针 (未使用)
 * @param[in] u8g2 指向 u8g2 实例的指针 (未使用)
 * @param[in] event 指向输入事件数据的指针
 * @return 无
 */
static void Page_Auto_Off_Action(Page_Base *page, u8g2_t *u8g2, const Input_Event_Data_t *event)
{
    Page_Auto_Off_Data_t *data = &g_page_auto_off_data;

    // 如果正在播放动画，则不响应任何操作
    if (data->state != AUTO_OFF_STATE_IDLE && data->state != AUTO_OFF_STATE_SHOW_MSG)
    {
        return;
    }
    // 如果正在显示反馈信息，只响应返回键
    if (data->state == AUTO_OFF_STATE_SHOW_MSG && event->event != INPUT_EVENT_BACK_PRESSED)
    {
        return;
    }

    switch (event->event)
    {
    case INPUT_EVENT_ENCODER:
    {
        int8_t old_index = data->selected_index;
        data->selected_index += event->value;

        // 修正索引，实现循环选择
        if (data->selected_index >= AUTO_OFF_ITEM_COUNT)
        {
            data->selected_index = data->selected_index % AUTO_OFF_ITEM_COUNT;
        }
        else if (data->selected_index < 0)
        {
            data->selected_index = (data->selected_index % AUTO_OFF_ITEM_COUNT + AUTO_OFF_ITEM_COUNT) % AUTO_OFF_ITEM_COUNT;
        }

        if (old_index == data->selected_index)
            break;

        int8_t old_viewport_top = data->viewport_top_index;
        // 判断新选中项是否需要滚动可视区域
        if (data->selected_index < data->viewport_top_index)
        {
            // 向上滚动，新选中项成为可视区域顶部
            data->viewport_top_index = data->selected_index;
        }
        else if (data->selected_index >= (data->viewport_top_index + VISIBLE_ITEMS))
        {
            // 向下滚动，新选中项成为可视区域底部
            data->viewport_top_index = data->selected_index - VISIBLE_ITEMS + 1;
        }

        // 启动高亮框移动画
        data->state = AUTO_OFF_STATE_ANIMATING_HIGHLIGHT;
        data->anim_start_y = data->anim_current_y;
        data->anim_target_y = LIST_TOP_Y + (data->selected_index - data->viewport_top_index) * AUTO_OFF_ITEM_HEIGHT;

        // 如果可视区域发生了变化（列表滚动），则动画时间更长，反之则更短
        if (old_viewport_top != data->viewport_top_index)
        {
            data->anim_duration = 200; // 列表滚动动画
        }
        else
        {
            data->anim_duration = 120; // 纯高亮框移动动画
        }

        data->anim_start_time = HAL_GetTick();
        break;
    }
    case INPUT_EVENT_COMFIRM_PRESSED:
        // 确认选择，保存设置
        g_app_settings.auto_off = data->selected_index;
        if (app_settings_save(&g_app_settings))
        {
            data->msg_text = "Settings Saved!";
        }
        else
        {
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

/**
 * @}
 */
