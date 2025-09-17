/**
 * @file      page_info.c
 * @brief     关于界面实现文件
 * @details   本文件定义了关于页面，用于显示应用名称、版本号等静态信息。
 * @author    SandOcean
 * @date      2025-09-16
 * @version   1.0
 * @copyright Copyright (c) 2025 SandOcean
 */

#include "app_display.h"
#include "app_config.h"
#include "input.h"

/* Private function prototypes -----------------------------------------------*/
static void Page_Info_Enter(Page_Base* page);
static void Page_Info_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset);
static void Page_Info_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event);

/* Public variables ----------------------------------------------------------*/
/**
 * @brief 关于页面的全局实例
 */
Page_Base g_page_info = {
    .enter = Page_Info_Enter,
    .exit = NULL,
    .loop = NULL, // 静态页面，不需要 loop 逻辑
    .draw = Page_Info_Draw,
    .action = Page_Info_Action,
    .page_name = "Info",
    .refresh_rate_ms = 10000, // 静态页面，不需要高刷新率
    .last_refresh_time = 0
};

/* Function implementations --------------------------------------------------*/

/**
 * @brief 关于页面进入函数
 * @param page 指向页面基类的指针 (未使用)
 */
static void Page_Info_Enter(Page_Base* page) {
    // 静态页面，进入时无需任何操作
}

/**
 * @brief 关于页面绘制函数
 * @param page 指向页面基类的指针 (未使用)
 * @param u8g2 指向u8g2实例的指针
 * @param x_offset 屏幕的X方向偏移
 * @param y_offset 屏幕的Y方向偏移
 */
static void Page_Info_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset) {
    /* 标题和Logo */
    u8g2_SetFont(u8g2, u8g2_font_open_iconic_app_2x_t); // 符号字体
    u8g2_DrawGlyph(u8g2, 0 + x_offset, 16 + y_offset, 0x0045); // 一个时钟符号作为Logo
    u8g2_SetFont(u8g2, INFO_FONT_BIG);
    u8g2_DrawStr(u8g2, 22 + x_offset, 14 + y_offset, APP_NAME);
    u8g2_DrawHLine(u8g2, 0 + x_offset, 18 + y_offset, 128);

    /* 基本信息 */
    u8g2_SetFont(u8g2, INFO_FONT_SMALL);
    u8g2_DrawStr(u8g2, 0 + x_offset, 30 + y_offset, "Firmware: " APP_VERSION);
    u8g2_DrawStr(u8g2, 0 + x_offset, 48 + y_offset, APP_COPYRIGHT);
    u8g2_DrawStr(u8g2, 0 + x_offset, 58 + y_offset, APP_AUTHOR);
}

/**
 * @brief 关于页面事件处理函数
 * @details 在关于页面，任何按键都视为返回操作。
 * @param page 指向页面基类的指针 (未使用)
 * @param u8g2 指向u8g2实例的指针 (未使用)
 * @param event 指向输入事件数据的指针
 */
static void Page_Info_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event) {
    switch (event->event) {
        case INPUT_EVENT_BACK_PRESSED:
        case INPUT_EVENT_COMFIRM_PRESSED:
        case INPUT_EVENT_ENCODER_PRESSED:
            Go_Back_Page();
            break;
        default:
            break;
    }
}