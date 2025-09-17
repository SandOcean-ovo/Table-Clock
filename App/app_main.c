/**
 * @file      app_main.c
 * @brief     应用层主文件
 * @details   本文件整合了项目应用层的各文件内容，并实现了自动熄屏逻辑。
 * @author    SandOcean
 * @date      2025-09-17
 * @version   1.1
 * @copyright Copyright (c) 2025 SandOcean
 */ 

#include "app_main.h"
#include "app_settings.h"
#include "DS3231.h"
#include "AHT20.h"
#include "input.h"
#include "app_display.h"
#include <stdbool.h>

/* Private variables ---------------------------------------------------------*/
static uint32_t last_activity_time = 0; // 记录最后一次用户活动的时间戳
static bool is_screen_on = true;        // 记录当前屏幕状态
static uint32_t auto_off_timeout_ms = 0;  // 自动熄屏的超时时间 (ms)
bool g_settings_load_failed = false;

/* Private function prototypes -----------------------------------------------*/
static void update_auto_off_timeout(void);
static void check_user_activity(void);
static void handle_auto_off(void);

/**
 * @brief 将设置中的索引转换为具体的超时毫秒数
 */
static void update_auto_off_timeout(void)
{
    // "Never", "30s", "1min", "5min", "10min"
    switch (g_app_settings.auto_off) {
        case 0: auto_off_timeout_ms = 0; break;          // 0: Never
        case 1: auto_off_timeout_ms = 30000; break;      // 1: 30s
        case 2: auto_off_timeout_ms = 60000; break;      // 2: 1min
        case 3: auto_off_timeout_ms = 300000; break;     // 3: 5min
        case 4: auto_off_timeout_ms = 600000; break;     // 4: 10min
        default: auto_off_timeout_ms = 0; break;
    }
}

/**
 * @brief 检查是否有用户输入活动
 */
static void check_user_activity(void)
{
    // 检查是否有输入事件
    if (input_count_events() > 0) {
        last_activity_time = HAL_GetTick(); // 重置活动时间

        // 如果屏幕已关闭，则本次输入仅用于唤醒
        if (!is_screen_on) {
            u8g2_SetPowerSave(&u8g2, 0); // 点亮屏幕
            is_screen_on = true;
            
            // 清除本次输入事件，防止其被页面逻辑处理
            input_clear_events();
        }
        // 每次活动后，重新加载一次超时设置，以防用户刚刚修改了它
        update_auto_off_timeout();
    }
}

/**
 * @brief 处理自动熄屏的计时和执行
 */
static void handle_auto_off(void)
{
    // 如果设置为 "Never" (0) 或屏幕已经关闭，则不做任何操作
    if (auto_off_timeout_ms == 0 || !is_screen_on) {
        return;
    }

    // 检查是否超时
    if (HAL_GetTick() - last_activity_time > auto_off_timeout_ms) {
        u8g2_SetPowerSave(&u8g2, 1); // 关闭屏幕
        is_screen_on = false;
        // 熄屏后，清空页面堆栈，返回到主时钟界面
        Page_Manager_Go_Home();
    }
}


void app_main_init(void)
{

    u8g2Init(&u8g2);
    DS3231_Init(&hi2c1);
    if (app_settings_init() == false) {
        g_settings_load_failed = true;
    }
    AHT20_Init(&hi2c1);
    Page_Manager_Init(&u8g2);
    input_init(&htim3, &htim2);

    // 初始化最后活动时间
    last_activity_time = HAL_GetTick();
    // 根据设置更新超时时间
    update_auto_off_timeout();
    is_screen_on = true; // 初始时屏幕点亮
}

void app_main_loop(void)
{
    // 1. 检查用户输入，并在有活动时重置计时器
    check_user_activity();

    // 2. 处理自动熄屏逻辑
    handle_auto_off();

    // 3. 只有在屏幕点亮时才更新和绘制UI
    if (is_screen_on) {
        Page_Manager_Loop();
    }
}