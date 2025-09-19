/**
 * @file      page_main.c
 * @brief     主页面实现文件
 * @details   本文件定义了主页面的行为，包括显示时间、日期、温湿度等信息。
 * @author    SandOcean
 * @date      2025-09-17
 * @version   1.1
 * @copyright Copyright (c) 2025 SandOcean
 */

#include "app_display.h"
#include "app_main.h" // 包含 app_main.h 以访问全局标志
#include "DS3231.h"
#include "AHT20.h"
#include "input.h"
#include <stdio.h>

/* Private types -------------------------------------------------------------*/
/**
 * @brief 主页面的私有数据结构体
 */
typedef struct
{
    Page_Base base; ///< 必须包含基类作为第一个成员
    // 私有数据: 用于存储需要显示的内容
    char time_str[12];      ///< 格式化的时间字符串
    char date_str[12];      ///< 格式化的日期字符串
    char week_str[5];       ///< 格式化的星期字符串
    char temp_humi_str[20]; ///< 格式化的温湿度字符串

    Time_t current_time;       ///< 当前时间数据
    float current_temp;        ///< 当前温度数据
    float current_humi;        ///< 当前湿度数据
    uint32_t last_update_time; ///< 上次更新温湿度的时间戳

    // 新增成员，用于处理设置加载失败的提示
    bool show_error_msg;
    uint32_t error_msg_start_time;

} Page_main_Data;

/* Private function prototypes -----------------------------------------------*/
static void Page_main_Enter(Page_Base *page);
static void Page_main_Loop(Page_Base *page);
static void Page_main_Draw(Page_Base *page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset);
static void Page_main_Action(Page_Base *page, u8g2_t *u8g2, const Input_Event_Data_t *event);

/* Private variables ---------------------------------------------------------*/
static Page_main_Data g_page_main_data; ///< 主页面的数据实例

/* Public variables ----------------------------------------------------------*/
/**
 * @brief 主页面的全局实例
 */
Page_Base g_page_main = {
    .enter = Page_main_Enter,
    .exit = NULL,
    .loop = Page_main_Loop,
    .draw = Page_main_Draw,
    .action = Page_main_Action,
    .page_name = "main",
    .parent_page = NULL,
    .refresh_rate_ms = 100, // 100ms刷新一次足够了
    .last_refresh_time = 0};

/* Function implementations --------------------------------------------------*/
/**
 * @brief 主页面进入函数
 * @param[in] page 指向页面基类的指针
 * @return 无
 */
static void Page_main_Enter(Page_Base *page)
{
    Page_main_Data *data = &g_page_main_data;
    data->last_update_time = 0; // 强制在每次进入主页时都立即刷新一次温湿度

    // 检查设置加载失败的全局标志
    if (g_settings_load_failed == true)
    {
        data->show_error_msg = true;
        data->error_msg_start_time = HAL_GetTick();
        // 将全局标志复位，这样下次返回主页时就不会再显示
        g_settings_load_failed = false;
    }
    else
    {
        data->show_error_msg = false;
    }

    Page_main_Loop(page); // 立即执行一次循环以填充数据
}

/**
 * @brief 主页面的逻辑循环函数
 * @details 负责更新时间和温湿度等数据。
 * @param[in] page 指向页面基类的指针
 * @return 无
 */
static void Page_main_Loop(Page_Base *page)
{
    Page_main_Data *data = &g_page_main_data;

    // 如果正在显示错误消息，检查是否超过3秒
    if (data->show_error_msg)
    {
        if (HAL_GetTick() - data->error_msg_start_time > 3000)
        {
            data->show_error_msg = false; // 3秒后停止显示
        }
    }

    DS3231_DST_GetTime(&data->current_time, g_app_settings.dst_enabled);

    // 每30秒更新一次温湿度
    if (data->last_update_time == 0 || HAL_GetTick() - data->last_update_time > 30000)
    {
        data->last_update_time = HAL_GetTick();
        AHT20_Read_Temp_Humi(&data->current_temp, &data->current_humi);
    }

    // 更新要显示的字符串数据
    sprintf(data->time_str, "%02d:%02d:%02d", data->current_time.hour, data->current_time.minute, data->current_time.second);
    sprintf(data->date_str, "%04d-%02d-%02d", data->current_time.year, data->current_time.month, data->current_time.day);

    const char *week_str_map[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
    if (data->current_time.week >= 1 && data->current_time.week <= 7)
    {
        strcpy(data->week_str, week_str_map[data->current_time.week - 1]);
    }

    sprintf(data->temp_humi_str, "T:%.1f\260C H:%.1f%%", data->current_temp, data->current_humi);
}

/**
 * @brief 主页面的绘制函数
 * @param[in] page 指向页面基类的指针
 * @param[in] u8g2 指向u8g2实例的指针
 * @param[in] x_offset 屏幕的X方向偏移
 * @param[in] y_offset 屏幕的Y方向偏移
 * @return 无
 */
static void Page_main_Draw(Page_Base *page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset)
{
    Page_main_Data *data = &g_page_main_data;

    /* 绘制时间 */
    u8g2_SetFont(u8g2, CLOCK_FONT);
    u8g2_uint_t time_width = u8g2_GetStrWidth(u8g2, data->time_str);
    u8g2_DrawStr(u8g2, (128 - time_width) / 2 + x_offset, 28 + y_offset, data->time_str);

    /* 绘制分割线 */
    u8g2_DrawHLine(u8g2, 0 + x_offset, 36 + y_offset, 128);

    /* 绘制日期和星期 */
    u8g2_SetFont(u8g2, DATE_TEMP_FONT);
    u8g2_DrawStr(u8g2, 2 + x_offset, 50 + y_offset, data->week_str);
    u8g2_uint_t date_width = u8g2_GetStrWidth(u8g2, data->date_str);
    u8g2_DrawStr(u8g2, (128 - date_width - 2) + x_offset, 50 + y_offset, data->date_str);

    /* 绘制温度和湿度 */
    u8g2_DrawStr(u8g2, 2 + x_offset, 62 + y_offset, data->temp_humi_str);

    /* 如果需要，在最上层绘制错误信息弹窗 */
    if (data->show_error_msg)
    {
        const char *msg = "Setting load failed";
        u8g2_SetFont(u8g2, PROMPT_FONT); // 假设 PROMPT_FONT 是一个已定义的字体
        uint16_t msg_w = u8g2_GetStrWidth(u8g2, msg);
        uint16_t box_w = msg_w + 10;
        uint16_t box_h = 16;
        uint16_t box_x = (u8g2_GetDisplayWidth(u8g2) - box_w) / 2;
        uint16_t box_y = (u8g2_GetDisplayHeight(u8g2) - box_h) / 2;

        u8g2_SetDrawColor(u8g2, 0); // 绘制黑色背景
        u8g2_DrawBox(u8g2, box_x, box_y, box_w, box_h);
        u8g2_SetDrawColor(u8g2, 1); // 绘制白色边框和文字
        u8g2_DrawFrame(u8g2, box_x, box_y, box_w, box_h);
        u8g2_DrawStr(u8g2, box_x + 5, box_y + 12, msg);
    }
}

/**
 * @brief 主页面的事件处理函数
 * @param[in] page 指向页面基类的指针
 * @param[in] u8g2 指向u8g2实例的指针
 * @param[in] event 指向输入事件数据的指针
 * @return 无
 */
static void Page_main_Action(Page_Base *page, u8g2_t *u8g2, const Input_Event_Data_t *event)
{
    if (event->event == INPUT_EVENT_COMFIRM_PRESSED)
    {
        Switch_Page(&g_page_main_menu); // 切换到菜单页
    }
}