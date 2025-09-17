/**
 * @file      page_language.c
 * @brief     语言设置页面
 * @details   本文件定义了“语言”设置菜单的UI和交互逻辑，
 *            并实现了高亮框移动的平滑动画效果。
 * @version   1.1
 * @date      2025-09-17
 * @author    SandOcean
 * @copyright Copyright (c) 2025 SandOcean
 */

#include "app_display.h"
#include "input.h"
#include "app_config.h"
#include "app_settings.h"

/**
 * @defgroup PageLanguage 语言设置页面
 * @{
 */

/* Private defines -----------------------------------------------------------*/
#define LANGUAGE_ITEM_COUNT 2       ///< 菜单项总数
#define LANGUAGE_ITEM_HEIGHT 16     ///< 每个菜单项的像素高度
#define LANGUAGE_TOP_Y 16           ///< 菜单列表顶部的Y坐标
#define LANGUAGE_LEFT_X 5           ///< 列表区域左侧的X坐标
#define LANGUAGE_WIDTH 118          ///< 列表区域的像素宽度

/* Private variables ---------------------------------------------------------*/
/**
 * @brief 菜单项文本数组
 */
static const char* menu_items[LANGUAGE_ITEM_COUNT] = {
    "English",
    "Chinese"
};

/**
 * @brief 页面动画状态枚举
 */
typedef enum {
    LANGUAGE_STATE_IDLE,        ///< 空闲状态，等待用户输入
    LANGUAGE_STATE_ANIMATING,   ///< 动画状态：移动高亮框
    LANGUAGE_STATE_SHOW_MSG     ///< 显示反馈信息状态
} Language_State_e;

/**
 * @brief 语言设置页面的私有数据结构体
 */
typedef struct {
    Page_Base base;             ///< 必须包含基类作为第一个成员
    int8_t selected_index;      ///< 当前选中的菜单项索引
    Language_State_e state;     ///< 当前页面的状态

    float anim_current_y;       ///< 动画插值计算出的当前Y坐标
    int16_t anim_start_y;       ///< 动画起始Y坐标
    int16_t anim_target_y;      ///< 动画目标Y坐标
    uint32_t anim_start_time;   ///< 动画开始的HAL Tick时间戳
    uint32_t anim_duration;     ///< 动画持续时间 (ms)

    uint32_t msg_start_time;    ///< 反馈信息显示的开始时间戳
    const char* msg_text;       ///< 指向要显示的反馈信息字符串
} Page_Language_Data_t;

static Page_Language_Data_t g_page_language_data; ///< 语言设置页面的数据实例

/* Private function prototypes -----------------------------------------------*/
static void Page_Language_Enter(Page_Base* page);
static void Page_Language_Loop(Page_Base* page);
static void Page_Language_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset);
static void Page_Language_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event);

/* Public variables ----------------------------------------------------------*/
/**
 * @brief 语言设置页面的全局实例
 */
Page_Base g_page_language = {
    .enter = Page_Language_Enter,
    .exit = NULL,
    .loop = Page_Language_Loop,
    .draw = Page_Language_Draw,
    .action = Page_Language_Action,
    .page_name = "Language",
    .refresh_rate_ms = 30, // ~33FPS
    .last_refresh_time = 0
};

/* Function implementations --------------------------------------------------*/

/**
 * @brief  页面进入函数
 * @details 当切换到此页面时被调用，用于初始化页面数据和状态。
 * @param  page: 指向页面基类的指针 (未使用)
 * @retval 无
 */
static void Page_Language_Enter(Page_Base* page) {
    Page_Language_Data_t* data = &g_page_language_data;
    data->state = LANGUAGE_STATE_IDLE;

    data->selected_index = g_app_settings.language;

    int16_t initial_y = LANGUAGE_TOP_Y + (data->selected_index * LANGUAGE_ITEM_HEIGHT);
    data->anim_current_y = initial_y;
    data->anim_target_y = initial_y;
    data->anim_start_y = initial_y;
}

/**
 * @brief  页面循环函数
 * @details 在每次页面刷新时被调用，用于处理动画更新和状态切换。
 * @param  page: 指向页面基类的指针 (未使用)
 * @retval 无
 */
static void Page_Language_Loop(Page_Base* page) {
    Page_Language_Data_t* data = &g_page_language_data;

    if (data->state == LANGUAGE_STATE_SHOW_MSG) {
        if (HAL_GetTick() - data->msg_start_time >= 1000) {
            data->state = LANGUAGE_STATE_IDLE;
            Go_Back_Page();
        }
        return;
    }

    if (data->state != LANGUAGE_STATE_ANIMATING) {
        return;
    }

    uint32_t elapsed = HAL_GetTick() - data->anim_start_time;
    if (elapsed >= data->anim_duration) {
        data->anim_current_y = data->anim_target_y;
        data->state = LANGUAGE_STATE_IDLE;
    } else {
        float progress = (float)elapsed / data->anim_duration;
        data->anim_current_y = data->anim_start_y + (data->anim_target_y - data->anim_start_y) * progress;
    }
}

/**
 * @brief  页面绘制函数
 * @details 负责在屏幕上绘制所有UI元素。
 * @param  page: 指向页面基类的指针 (未使用)
 * @param  u8g2: 指向 u8g2 实例的指针
 * @param  x_offset: 屏幕的X方向偏移 (未使用)
 * @param  y_offset: 屏幕的Y方向偏移 (未使用)
 * @retval 无
 */
static void Page_Language_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset) {
    Page_Language_Data_t* data = &g_page_language_data;

    u8g2_SetFont(u8g2, MENU_FONT);
    u8g2_SetDrawColor(u8g2, 1);
    for (int i = 0; i < LANGUAGE_ITEM_COUNT; i++) {
        u8g2_DrawStr(u8g2, 15 + x_offset, (i * LANGUAGE_ITEM_HEIGHT) + LANGUAGE_TOP_Y + 12 + y_offset, menu_items[i]);
    }

    int16_t clip_x0 = LANGUAGE_LEFT_X + x_offset;
    int16_t clip_y0 = (int16_t)data->anim_current_y + y_offset;
    u8g2_SetClipWindow(u8g2, clip_x0, clip_y0, clip_x0 + LANGUAGE_WIDTH, clip_y0 + LANGUAGE_ITEM_HEIGHT);
    u8g2_SetDrawColor(u8g2, 1); 
    u8g2_DrawBox(u8g2, clip_x0, clip_y0, LANGUAGE_WIDTH, LANGUAGE_ITEM_HEIGHT);
    u8g2_SetDrawColor(u8g2, 0);
    for (int i = 0; i < LANGUAGE_ITEM_COUNT; i++) {
        u8g2_DrawStr(u8g2, 15 + x_offset, (i * LANGUAGE_ITEM_HEIGHT) + LANGUAGE_TOP_Y + 12 + y_offset, menu_items[i]);
    }
    u8g2_SetMaxClipWindow(u8g2);
    u8g2_SetDrawColor(u8g2, 1);

    if (data->state == LANGUAGE_STATE_SHOW_MSG) {
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

/**
 * @brief  页面事件处理函数
 * @details 处理来自输入设备的事件，如旋钮旋转、按键按下等。
 * @param  page: 指向页面基类的指针 (未使用)
 * @param  u8g2: 指向 u8g2 实例的指针 (未使用)
 * @param  event: 指向输入事件数据的指针
 * @retval 无
 */
static void Page_Language_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event) {
    Page_Language_Data_t* data = &g_page_language_data;

    if (data->state == LANGUAGE_STATE_SHOW_MSG || data->state == LANGUAGE_STATE_ANIMATING) {
        return;
    }

    switch (event->event) {
        case INPUT_EVENT_ENCODER: {
            int8_t old_index = data->selected_index;
            data->selected_index += event->value;
            if (data->selected_index >= LANGUAGE_ITEM_COUNT) data->selected_index = 0;
            if (data->selected_index < 0) data->selected_index = LANGUAGE_ITEM_COUNT - 1;

            if (old_index != data->selected_index) {
                data->state = LANGUAGE_STATE_ANIMATING;
                data->anim_start_time = HAL_GetTick();
                data->anim_duration = 150;
                data->anim_start_y = data->anim_current_y;
                data->anim_target_y = LANGUAGE_TOP_Y + (data->selected_index * LANGUAGE_ITEM_HEIGHT);
            }
            break;
        }
        case INPUT_EVENT_COMFIRM_PRESSED:
            g_app_settings.language = data->selected_index;
            if (app_settings_save(&g_app_settings)) {
                data->msg_text = "Settings Saved!";
            } else {
                data->msg_text = "Save Failed!";
            }
            data->state = LANGUAGE_STATE_SHOW_MSG;
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
