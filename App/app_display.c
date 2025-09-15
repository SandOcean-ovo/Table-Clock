/**
 * @file app_display.c
 * @brief 应用显示函数
 * @details 本文件定义了应用的显示功能
 * @author SandOcean
 * @date 2025-08-25
 * @version 1.0
 */

#include "app_display.h"
#include "u8g2_stm32_hal.h"
#include <string.h>
#include "input.h"

void draw_info_page(u8g2_t *u8g2) 
{

    /* 1. 标题和Logo */
    u8g2_SetFont(u8g2, u8g2_font_open_iconic_app_2x_t); // 符号字体
    u8g2_DrawGlyph(u8g2, 0, 16, 0x0045); // 一个时钟符号作为Logo
    u8g2_SetFont(u8g2, u8g2_font_profont12_tf);
    u8g2_DrawStr(u8g2, 22, 14, APP_NAME);
    u8g2_DrawHLine(u8g2, 0, 18, 128);

    /* 2. 基本信息 */
    u8g2_SetFont(u8g2, u8g2_font_profont10_tf);
    u8g2_DrawStr(u8g2, 0, 28, "Firmware: " APP_VERSION);
    
    u8g2_DrawStr(u8g2, 0, 48, APP_COPYRIGHT);
    u8g2_DrawStr(u8g2, 0, 58, APP_AUTHOR);

}

// ====================================================================
//            Part 1: 页面管理器内部状态 (Manager Internals)
// ====================================================================

// 管理器自身的状态
typedef enum {
    MANAGER_STATE_IDLE,     // 静止状态，按页面的刷新率工作
    MANAGER_STATE_ANIMATING // 动画状态，以最快速度工作
} Manager_State_e;

/**
 * @brief 页面管理器内部状态结构体
 */
static struct {
    u8g2_t* u8g2;                   // 指向u8g2实例的指针
    Page_Base* current_page;        // 指向当前活动页面的指针

    // --- 动画相关状态 ---
    Manager_State_e state;          // 管理器当前状态
    Page_Base* page_from;           // 动画的来源页面
    Page_Base* page_to;             // 动画的目标页面
    uint32_t anim_start_time;       // 动画开始的时间戳
    uint32_t anim_duration;         // 动画总时长
    
    // --- 页面历史堆栈 ---
    // ... (您的历史堆栈代码) ...
} g_page_manager;

// ====================================================================
//            Part 2: 页面具体实现 (Page Implementations)
// ====================================================================
// ... (所有页面的 draw, loop, action 函数和 g_page_... 定义保持不变) ...

// ====================================================================
//            Part 3: 页面管理器API实现 (Manager API)
// ====================================================================

// 初始化函数
void Page_Manager_Init(u8g2_t* u8g2_ptr) {
    g_page_manager.u8g2 = u8g2_ptr;

    // 设置初始页面
    g_page_manager.current_page = &g_page_main;
    if (g_page_manager.current_page->enter) {
        g_page_manager.current_page->enter(g_page_manager.current_page);
    }
}

/**
 * @brief 切换到指定的页面 (带动画)
 * @param new_page 目标页面
 */
void Switch_Page(Page_Base* new_page)
{
    if (!new_page || new_page == g_page_manager.current_page || g_page_manager.state == MANAGER_STATE_ANIMATING) {
        return;
    }

    // 调用旧页面的退出函数
    if (g_page_manager.current_page && g_page_manager.current_page->exit) {
        g_page_manager.current_page->exit(g_page_manager.current_page);
    }

    // --- 启动动画 ---
    g_page_manager.page_from = g_page_manager.current_page;
    g_page_manager.page_to = new_page;
    g_page_manager.anim_start_time = HAL_GetTick();
    g_page_manager.anim_duration = 250; // 动画持续250ms
    g_page_manager.state = MANAGER_STATE_ANIMATING; // 进入动画状态
    
    // 注意：不再在这里切换 current_page 和调用 enter 函数，这些都推迟到动画结束后
}

/**
 * @brief 页面管理器的主循环函数 (升级版)
 */
void Page_Manager_Loop(void)
{
    // 1. 优先处理动画状态
    if (g_page_manager.state == MANAGER_STATE_ANIMATING) {
        uint32_t elapsed = HAL_GetTick() - g_page_manager.anim_start_time;

        // --- 动画结束 ---
        if (elapsed >= g_page_manager.anim_duration) {
            g_page_manager.state = MANAGER_STATE_IDLE; // 恢复静止状态
            g_page_manager.current_page = g_page_manager.page_to; // 正式切换页面
            
            // 调用新页面的进入函数
            if (g_page_manager.current_page && g_page_manager.current_page->enter) {
                g_page_manager.current_page->enter(g_page_manager.current_page);
            }
            // 动画结束后，立即强制刷新一次最终画面
            if (g_page_manager.current_page && g_page_manager.current_page->draw) {
                 u8g2_FirstPage(g_page_manager.u8g2);
                 do {
                     g_page_manager.current_page->draw(g_page_manager.current_page, g_page_manager.u8g2, 0, 0);
                 } while (u8g2_NextPage(g_page_manager.u8g2));
            }
            return; // 本次循环结束
        }

        // --- 动画进行中 ---
        float progress = (float)elapsed / g_page_manager.anim_duration;
        int16_t screen_width = u8g2_GetDisplayWidth(g_page_manager.u8g2);
        int16_t from_x = -(int16_t)(screen_width * progress);
        int16_t to_x = screen_width - (int16_t)(screen_width * progress);

        // 同时绘制旧页面和新页面，并施加位移
        u8g2_FirstPage(g_page_manager.u8g2);
        do {
            if (g_page_manager.page_from && g_page_manager.page_from->draw) {
                g_page_manager.page_from->draw(g_page_manager.page_from, g_page_manager.u8g2, from_x, 0);
            }
            if (g_page_manager.page_to && g_page_manager.page_to->draw) {
                g_page_manager.page_to->draw(g_page_manager.page_to, g_page_manager.u8g2, to_x, 0);
            }
        } while (u8g2_NextPage(g_page_manager.u8g2));

    } 
    // 2. 如果是静止状态，则按原计划工作
    else { 
        Page_Base* current = g_page_manager.current_page;
        if (!current) return;

        // 处理输入事件
        Input_Event_Data_t event;
        if (input_get_event(&event)) {
            if (current->action) {
                // 传递整个事件结构体的指针！
                current->action(current, g_page_manager.u8g2, &event);
            }
        }

        // 调用当前页面的循环逻辑
        if (current->loop) {
            current->loop(current);
        }

        // 根据页面的 refresh_rate_ms 判断是否需要重绘
        uint32_t now = HAL_GetTick();
        if (now - current->last_refresh_time >= current->refresh_rate_ms) {
            current->last_refresh_time = now;
            
            if (current->draw) {
                u8g2_FirstPage(g_page_manager.u8g2);
                do {
                    current->draw(current, g_page_manager.u8g2, 0, 0);
                } while (u8g2_NextPage(g_page_manager.u8g2));
            }
        }
    }
}

// 返回功能
void Go_Back_Page(void) {
    if (g_page_manager.current_page && g_page_manager.current_page->parent_page) {
        Switch_Page(g_page_manager.current_page->parent_page);
    }
}