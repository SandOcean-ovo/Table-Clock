/**
 * @file page_main_menu.c
 * @brief 菜单页面实现文件
 * @details 本文件定义了菜单页面的行为。
 * @author SandOcean
 * @date 2025-09-15
 * @version 1.0
 */

#include "app_display.h"
#include "DS3231.h"
#include "AHT20.h"
#include "input.h" // 需要包含 input.h 来使用事件枚举

#define MENU_ITEM_COUNT 3
#define MENU_ITEM_HEIGHT 16
#define MENU_TOP_Y 8
#define MENU_LEFT_X 2
#define MENU_WIDTH 118

// 1. 定义页面私有数据结构体

typedef enum {
    MENU_STATE_IDLE,      // 静止状态
    MENU_STATE_ANIMATING  // 动画状态
} Menu_State_e;

typedef struct
{
    Page_Base base; // 必须包含基类作为第一个成员
    // 私有数据: 当前选择索引
    int8_t selected_index;
    // --- 内部动画状态机 ---
    Menu_State_e state;         // 菜单自身的动画状态
    float anim_current_y;       // 高亮框当前的Y坐标 (使用float以实现平滑移动)
    int16_t anim_target_y;      // 高亮框的目标Y坐标
    uint32_t anim_start_time;   // 动画开始时间
    uint32_t anim_duration;     // 动画总时长
} Page_main_menu_Data;

// 2. 声明本页面的函数
static void Page_main_menu_Enter(Page_Base* page);
static void Page_main_menu_Loop(Page_Base* page);
static void Page_main_menu_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset);
static void Page_main_menu_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event);

// 3. 定义页面全局实例 
static Page_main_menu_Data g_page_main_menu_data;
Page_Base g_page_main_menu = {
    .enter = Page_main_menu_Enter,
    .exit = NULL,
    .loop = Page_main_menu_Loop, // 我们现在需要Loop函数来驱动动画
    .draw = Page_main_menu_Draw,
    .action = Page_main_menu_Action,
    .page_name = "main_menu",
    .parent_page = &g_page_main, // 父页面应该是主时钟页
    .refresh_rate_ms = 30, // 提高刷新率以保证动画流畅 (~33FPS)
    .last_refresh_time = 0
};

// 4. 函数具体实现
static void Page_main_menu_Enter(Page_Base* page) {
    Page_main_menu_Data* data = &g_page_main_menu_data;
    data->state = MENU_STATE_IDLE;
    data->selected_index = 0;
    // 初始化高亮框的Y坐标，避免第一次进入时跳动
    data->anim_current_y = MENU_TOP_Y + (data->selected_index * (MENU_ITEM_HEIGHT));
}

static void Page_main_menu_Loop(Page_Base* page) {
    Page_main_menu_Data* data = &g_page_main_menu_data;

    // 如果不处于动画状态，则什么都不做
    if (data->state != MENU_STATE_ANIMATING) {
        return;
    }

    uint32_t elapsed = HAL_GetTick() - data->anim_start_time;

    if (elapsed >= data->anim_duration) {
        // 动画结束
        data->anim_current_y = data->anim_target_y; // 精确停在目标位置
        data->state = MENU_STATE_IDLE;
    } else {
        // 动画进行中，使用线性插值计算当前Y坐标
        float progress = (float)elapsed / data->anim_duration;
        float start_y = data->anim_current_y;
        data->anim_current_y = start_y + (data->anim_target_y - start_y) * progress;
    }
}

static void Page_main_menu_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset) {
    Page_main_menu_Data* data = &g_page_main_menu_data;
    const char* menu_items[] = {"Display", "Time Set", "Info"};

    // --- 准备工作 ---
    u8g2_SetFont(u8g2, MENU_FONT);
    
    // --- 步骤 1: 绘制基础层 ---
    // 将所有菜单项都以“正常”颜色绘制一遍
    u8g2_SetDrawColor(u8g2, 1);
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        // 使用你原来的坐标计算方式
        u8g2_DrawStr(u8g2, 15 + x_offset, (i * MENU_ITEM_HEIGHT) + MENU_TOP_Y + 12 + y_offset, menu_items[i]);
    }

    // --- 步骤 2: 定义并设置裁剪窗口 ---
    // 这个窗口就是你的动画高亮框的实时位置和大小
    int16_t clip_x0 = MENU_LEFT_X + x_offset;
    int16_t clip_y0 = (int16_t)data->anim_current_y + y_offset;
    int16_t clip_x1 = clip_x0 + MENU_WIDTH;
    int16_t clip_y1 = clip_y0 + MENU_ITEM_HEIGHT;
    
    // 设置裁剪窗口，之后的所有绘图操作只在这个矩形内有效
    u8g2_SetClipWindow(u8g2, clip_x0, clip_y0, clip_x1, clip_y1);

    // --- 步骤 3: 在裁剪窗口内绘制高亮的反色层 ---
    // a. 绘制一个实心框作为反色背景。因为它在裁剪窗口内，所以大小和位置与窗口完全重合。
    u8g2_SetDrawColor(u8g2, 1); 
    u8g2_DrawBox(u8g2, clip_x0, clip_y0, MENU_WIDTH, MENU_ITEM_HEIGHT);

    // b. 将绘制颜色设为背景色(0)
    u8g2_SetDrawColor(u8g2, 0);
    // c. 再次绘制所有的菜单项。
    //    由于裁剪窗口的存在，只有落在高亮框内的那部分文字才会被真正画出来（并覆盖掉白色背景，形成反色效果）。
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        u8g2_DrawStr(u8g2, 15 + x_offset, (i * MENU_ITEM_HEIGHT) + MENU_TOP_Y + 12 + y_offset, menu_items[i]);
    }

    // --- 步骤 4: 【至关重要】恢复裁剪窗口 ---
    // 撤销裁剪窗口的限制，恢复到全屏绘制模式
    u8g2_SetMaxClipWindow(u8g2);

    // --- 步骤 5: 恢复默认绘制颜色，以备后续使用 ---
    u8g2_SetDrawColor(u8g2, 1);
}

static void Page_main_menu_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event) {
    Page_main_menu_Data* data = &g_page_main_menu_data;

    // 如果正在动画中，则忽略新的输入，防止动画错乱
    if (data->state == MENU_STATE_ANIMATING) {
        return;
    }

    switch (event->event) {
        case INPUT_EVENT_ENCODER: {
            int8_t old_index = data->selected_index;
            data->selected_index += event->value;

            // 循环边界检查
            if (data->selected_index >= MENU_ITEM_COUNT) {
                data->selected_index = 0;
            }
            if (data->selected_index < 0) {
                // 修正：应该循环到最后一项，即 MENU_ITEM_COUNT - 1
                data->selected_index = MENU_ITEM_COUNT - 1;
            }

            // 如果选项改变了，则启动动画
            if (old_index != data->selected_index) {
                data->state = MENU_STATE_ANIMATING;
                data->anim_start_time = HAL_GetTick();
                data->anim_duration = 150; // 动画持续150ms
                // 注意：动画的起点是上一次动画结束的位置，即 anim_current_y
                data->anim_target_y = MENU_TOP_Y + (data->selected_index * (MENU_ITEM_HEIGHT));
            }
            break;
        }
        case INPUT_EVENT_COMFIRM_PRESSED:
            // 根据选中的是哪一项，切换到不同的子页面
            if (data->selected_index == 0) Switch_Page(&g_page_display);
            if (data->selected_index == 1) Switch_Page(&g_page_time_set);
            if (data->selected_index == 2) Switch_Page(&g_page_info);
            break;
        case INPUT_EVENT_BACK_PRESSED:
            Go_Back_Page(); // 使用框架提供的返回功能
            break;
    }
}