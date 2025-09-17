/**
 * @file page_display.c
 * @brief 显示设置子菜单页面
 * @details 本文件定义了“显示”设置的子菜单，包含“语言”和“自动熄屏”选项。
 * @author SandOcean
 * @date 2025-09-16
 * @version 1.0
 */

#include "app_display.h"
#include "input.h"

// --- 1. 页面私有定义 ---
#define DISPLAY_MENU_ITEM_COUNT 2
#define DISPLAY_MENU_ITEM_HEIGHT 16
#define DISPLAY_MENU_TOP_Y 8
#define DISPLAY_MENU_LEFT_X 5
#define DISPLAY_MENU_WIDTH 118

// 菜单项的字符串
static const char* menu_items[DISPLAY_MENU_ITEM_COUNT] = {
    "Language",
    "Auto-Off"
};

// 菜单动画状态
typedef enum {
    DISPLAY_MENU_STATE_IDLE,
    DISPLAY_MENU_STATE_ANIMATING
} Display_Menu_State_e;

// --- 2. 定义页面私有数据结构体 ---
typedef struct {
    int8_t selected_index;          // 当前选中的菜单项索引
    Display_Menu_State_e state;     // 菜单的动画状态 (静止/动画中)
    float anim_current_y;           // 高亮框当前的Y坐标 (用于动画插值)
    int16_t anim_start_y;           // 【新增】高亮框动画的起始Y坐标
    int16_t anim_target_y;          // 高亮框动画的目标Y坐标
    uint32_t anim_start_time;       // 动画开始的系统时间
    uint32_t anim_duration;         // 动画持续时间 (ms)
} Page_Display_Data_t;

// --- 3. 声明并初始化页面私有数据 ---
static Page_Display_Data_t g_page_display_data;

// --- 4. 声明本页面的函数 ---
static void Page_Display_Enter(Page_Base* page);
static void Page_Display_Loop(Page_Base* page);
static void Page_Display_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset);
static void Page_Display_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event);

// --- 5. 定义页面全局实例 ---
Page_Base g_page_display = {
    .enter = Page_Display_Enter,
    .exit = NULL,
    .loop = Page_Display_Loop,
    .draw = Page_Display_Draw,
    .action = Page_Display_Action,
    .page_name = "Display",
    .refresh_rate_ms = 30, // 动画需要高刷新率 (~33FPS)
    .last_refresh_time = 0
};

// --- 6. 函数具体实现 ---

/**
 * @brief 页面进入函数
 */
static void Page_Display_Enter(Page_Base* page) {
    Page_Display_Data_t* data = &g_page_display_data;
    data->state = DISPLAY_MENU_STATE_IDLE;
    data->selected_index = 0;

    // 强制将动画的当前位置和目标位置都设置为第一个选项的坐标
    int16_t initial_y = DISPLAY_MENU_TOP_Y + (data->selected_index * DISPLAY_MENU_ITEM_HEIGHT);
    data->anim_current_y = initial_y;
    data->anim_target_y = initial_y;
}

/**
 * @brief 页面循环逻辑函数 (处理动画)
 */
static void Page_Display_Loop(Page_Base* page) {
    Page_Display_Data_t* data = &g_page_display_data;

    if (data->state != DISPLAY_MENU_STATE_ANIMATING) {
        return;
    }

    uint32_t elapsed = HAL_GetTick() - data->anim_start_time;

    if (elapsed >= data->anim_duration) {
        // 动画结束
        data->anim_current_y = data->anim_target_y;
        data->state = DISPLAY_MENU_STATE_IDLE;
    } else {
        // 动画进行中，使用线性插值计算当前Y坐标
        float progress = (float)elapsed / data->anim_duration;
        
        // 【修正】使用保存好的 anim_start_y 作为起点进行插值
        data->anim_current_y = data->anim_start_y + (data->anim_target_y - data->anim_start_y) * progress;
    }
}

/**
 * @brief 页面绘制函数
 */
static void Page_Display_Draw(Page_Base* page, u8g2_t *u8g2, int16_t x_offset, int16_t y_offset) {
    Page_Display_Data_t* data = &g_page_display_data;

    // --- 步骤 1: 绘制基础层 (所有正常显示的文字) ---
    u8g2_SetFont(u8g2, MENU_FONT);
    u8g2_SetDrawColor(u8g2, 1);
    for (int i = 0; i < DISPLAY_MENU_ITEM_COUNT; i++) {
        u8g2_DrawStr(u8g2, 15 + x_offset, (i * DISPLAY_MENU_ITEM_HEIGHT) + DISPLAY_MENU_TOP_Y + 12 + y_offset, menu_items[i]);
    }

    // --- 步骤 2: 设置裁剪窗口以绘制高亮反色层 ---
    int16_t clip_x0 = DISPLAY_MENU_LEFT_X + x_offset;
    int16_t clip_y0 = (int16_t)data->anim_current_y + y_offset;
    int16_t clip_x1 = clip_x0 + DISPLAY_MENU_WIDTH;
    int16_t clip_y1 = clip_y0 + DISPLAY_MENU_ITEM_HEIGHT;
    u8g2_SetClipWindow(u8g2, clip_x0, clip_y0, clip_x1, clip_y1);

    // --- 步骤 3: 在裁剪窗口内绘制反色内容 ---
    // a. 绘制实心框作为反色背景
    u8g2_SetDrawColor(u8g2, 1); 
    u8g2_DrawBox(u8g2, clip_x0, clip_y0, DISPLAY_MENU_WIDTH, DISPLAY_MENU_ITEM_HEIGHT);

    // b. 设置绘制颜色为背景色(0)，再次绘制所有文字
    u8g2_SetDrawColor(u8g2, 0);
    for (int i = 0; i < DISPLAY_MENU_ITEM_COUNT; i++) {
        u8g2_DrawStr(u8g2, 15 + x_offset, (i * DISPLAY_MENU_ITEM_HEIGHT) + DISPLAY_MENU_TOP_Y + 12 + y_offset, menu_items[i]);
    }

    // --- 步骤 4: 恢复裁剪窗口和绘制颜色 ---
    u8g2_SetMaxClipWindow(u8g2);
    u8g2_SetDrawColor(u8g2, 1);
}

/**
 * @brief 页面输入事件处理函数
 */
static void Page_Display_Action(Page_Base* page, u8g2_t *u8g2, const Input_Event_Data_t* event) {
    Page_Display_Data_t* data = &g_page_display_data;

    if (data->state == DISPLAY_MENU_STATE_ANIMATING) {
        return; // 动画期间忽略输入
    }

    switch (event->event) {
        case INPUT_EVENT_ENCODER: {
            int8_t old_index = data->selected_index;
            data->selected_index += event->value;

            // 循环边界检查
            if (data->selected_index >= DISPLAY_MENU_ITEM_COUNT) {
                data->selected_index = 0;
            }
            if (data->selected_index < 0) {
                data->selected_index = DISPLAY_MENU_ITEM_COUNT - 1;
            }

            // 如果选项改变，则启动动画
            if (old_index != data->selected_index) {
                data->state = DISPLAY_MENU_STATE_ANIMATING;
                data->anim_start_time = HAL_GetTick();
                data->anim_duration = 150; // 动画时长
                data->anim_start_y = data->anim_current_y; // 保存动画起始位置
                data->anim_target_y = DISPLAY_MENU_TOP_Y + (data->selected_index * DISPLAY_MENU_ITEM_HEIGHT);
            }
            break;
            break;
        }
        case INPUT_EVENT_COMFIRM_PRESSED:
            if(data->selected_index == 1) {
                Switch_Page(&g_page_auto_off); // 切换到自动熄屏设置页面
            } 
            else if(data->selected_index == 0) {
                Switch_Page(&g_page_language); // 切换到语言设置页面
            }
            break;

        case INPUT_EVENT_BACK_PRESSED:
            Go_Back_Page(); // 返回上一级菜单
            break;
        
        default:
            break;
    }
}