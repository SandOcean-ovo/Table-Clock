/**
 * @file page_main.c
 * @brief 主页面实现文件
 * @details 本文件定义了主页面的行为，包括显示时间、日期、温湿度等信息。
 * @author SandOcean
 * @date 2025-09-15
 * @version 1.0
 */

#include "app_display.h"
#include "DS3231.h"
#include "AHT20.h"
#include "input.h" 

// 1. 定义页面私有数据结构体
typedef struct
{
    Page_Base base; // 必须包含基类作为第一个成员
    // 私有数据: 用于存储需要显示的内容
    char time_str[12];
    char date_str[12];
    char week_str[5];
    char temp_humi_str[20];

    Time_t current_time;
    float current_temp;
    float current_humi;
    uint32_t last_update_time; 

} Page_main_Data;

// 2. 声明本页面的函数
static void Page_main_Enter(Page_Base *page);
static void Page_main_Loop(Page_Base *page);
static void Page_main_Draw(Page_Base *page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset);
static void Page_main_Action(Page_Base *page, u8g2_t *u8g2, const Input_Event_Data_t* event);

// 3. 定义页面全局实例
// 注意：这里我们只定义一个 Page_Base 实例。
// Page_main_Data 实例将在 Page_Manager 中管理或作为静态变量。
// 为了简单起见，我们先用一个静态全局变量。
static Page_main_Data g_page_main_data;

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

// 4. 函数具体实现
static void Page_main_Enter(Page_Base *page)
{
    Page_main_Data *data = &g_page_main_data;
    data->last_update_time = 0; // 强制在每次进入主页时都立即刷新一次温湿度

    // 立即执行一次数据更新逻辑
    Page_main_Loop(page);
}

/**
 * @brief 主页面的逻辑循环函数
 * @details 只负责更新数据，不负责绘制
 */
static void Page_main_Loop(Page_Base *page)
{
    // 将通用的 page 指针转换为本页面专属的数据指针
    Page_main_Data *data = &g_page_main_data;

    // 获取硬件数据
    DS3231_GetTime(&data->current_time);

    // 【修改】使用数据结构中的变量，并优化逻辑
    if (data->last_update_time == 0 || HAL_GetTick() - data->last_update_time > 30000)
    {
        data->last_update_time = HAL_GetTick();
        AHT20_Read_Temp_Humi(&data->current_temp, &data->current_humi);
    }

    // 更新要显示的字符串数据
    sprintf(data->time_str, "%02d:%02d:%02d", data->current_time.hour, data->current_time.minute, data->current_time.second);
    sprintf(data->date_str, "%04d-%02d-%02d", data->current_time.year, data->current_time.month, data->current_time.day);

    const char *week_str_map[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
    // 注意：DS3231的星期范围可能是1-7，请确保 week 成员的值与数组索引对应
    if (data->current_time.week >= 1 && data->current_time.week <= 7) {
        strcpy(data->week_str, week_str_map[data->current_time.week - 1]);
    }

    sprintf(data->temp_humi_str, "T:%.1f\260C H:%.1f%%", data->current_temp, data->current_humi);
}

/**
 * @brief 主页面的绘制函数
 * @details 只负责绘制，不处理逻辑
 */
static void Page_main_Draw(Page_Base *page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset)
{
    // 将通用的 page 指针转换为本页面专属的数据指针
    Page_main_Data *data = &g_page_main_data;

    /* --- 1. 绘制时间 --- */
    u8g2_SetFont(u8g2, CLOCK_FONT);
    u8g2_uint_t time_width = u8g2_GetStrWidth(u8g2, data->time_str);
    u8g2_DrawStr(u8g2, (128 - time_width) / 2 + x_offset, 28 + y_offset, data->time_str);

    /* --- 2. 绘制分割线 --- */
    u8g2_DrawHLine(u8g2, 0 + x_offset, 36 + y_offset, 128);

    /* --- 3. 绘制日期和星期 --- */
    u8g2_SetFont(u8g2, DATE_TEMP_FONT);
    u8g2_DrawStr(u8g2, 2 + x_offset, 50 + y_offset, data->week_str);
    u8g2_uint_t date_width = u8g2_GetStrWidth(u8g2, data->date_str);
    u8g2_DrawStr(u8g2, (128 - date_width - 2) + x_offset, 50 + y_offset, data->date_str);

    /* --- 4. 绘制温度和湿度 --- */
    u8g2_DrawStr(u8g2, 2 + x_offset, 62 + y_offset, data->temp_humi_str);
}

static void Page_main_Action(Page_Base *page, u8g2_t *u8g2, const Input_Event_Data_t* event)
{
    if (event->event == INPUT_EVENT_COMFIRM_PRESSED)
    {
        // 假设 g_page_main_menu 已经在 app_display.h 中声明
        Switch_Page(&g_page_main_menu); // 切换到菜单页
    }
}