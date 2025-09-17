/**
 * @file page_info.c
 * @brief 关于界面实现文件
 * @details 本文件定义了关于页面。
 * @author SandOcean
 * @date 2025-09-16
 * @version 1.0
 */

#include "app_display.h"
#include "app_config.h"
#include "input.h"

// --- 1. 定义页面私有数据结构体 ---
// 由于本页面是纯静态展示，没有任何需要运行时改变的数据，所以我们可以不定义数据结构体。


// --- 2. 声明本页面的函数 ---
static void Page_Info_Enter(Page_Base* page);
static void Page_Info_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset);
static void Page_Info_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event);

// --- 3. 定义页面全局实例 ---
// 由于没有私有数据，我们甚至不需要一个单独的全局数据变量。
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

// --- 4. 函数具体实现 ---

static void Page_Info_Enter(Page_Base* page) {
    // 静态页面，进入时无需任何操作
}

static void Page_Info_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset) {
    /* 1. 标题和Logo */
    u8g2_SetFont(u8g2, u8g2_font_open_iconic_app_2x_t); // 符号字体
    u8g2_DrawGlyph(u8g2, 0 + x_offset, 16 + y_offset, 0x0045); // 一个时钟符号作为Logo
    u8g2_SetFont(u8g2, INFO_FONT_BIG);
    u8g2_DrawStr(u8g2, 22 + x_offset, 14 + y_offset, APP_NAME);
    u8g2_DrawHLine(u8g2, 0 + x_offset, 18 + y_offset, 128);

    /* 2. 基本信息 */
    u8g2_SetFont(u8g2, INFO_FONT_SMALL);
    u8g2_DrawStr(u8g2, 0 + x_offset, 30 + y_offset, "Firmware: " APP_VERSION);
    u8g2_DrawStr(u8g2, 0 + x_offset, 48 + y_offset, APP_COPYRIGHT);
    u8g2_DrawStr(u8g2, 0 + x_offset, 58 + y_offset, APP_AUTHOR);
}

static void Page_Info_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event) {
    // 在关于页面，任何按键（确认、返回）都视为返回操作
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