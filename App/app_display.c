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
#include <stdbool.h>
#include "input.h"

#define PAGE_HISTORY_MAX_DEPTH 8 // 定义堆栈最大深度

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
    Page_Base* history_stack[PAGE_HISTORY_MAX_DEPTH]; // 存储历史页面的数组
    int8_t history_depth;                             // 当前堆栈深度 (或叫栈顶指针)

} g_page_manager;

static void _Switch_Page_Internal(Page_Base* new_page, bool record_history) {
    if (!new_page || new_page == g_page_manager.current_page || g_page_manager.state == MANAGER_STATE_ANIMATING) {
        return;
    }

    if (record_history) {
        if (g_page_manager.current_page && g_page_manager.history_depth < PAGE_HISTORY_MAX_DEPTH) {
            g_page_manager.history_stack[g_page_manager.history_depth] = g_page_manager.current_page;
            g_page_manager.history_depth++;
        }
    }

    if (g_page_manager.current_page && g_page_manager.current_page->exit) {
        g_page_manager.current_page->exit(g_page_manager.current_page);
    }

    // 在这里调用新页面的 enter 函数，解决动画过程和结束后显示不一致的问题
    if (new_page->enter) {
        new_page->enter(new_page);
    }

    g_page_manager.page_from = g_page_manager.current_page;
    g_page_manager.page_to = new_page;
    g_page_manager.anim_start_time = HAL_GetTick();
    g_page_manager.anim_duration = 250;
    g_page_manager.state = MANAGER_STATE_ANIMATING;
}

// 初始化函数
void Page_Manager_Init(u8g2_t* u8g2_ptr) {
    g_page_manager.u8g2 = u8g2_ptr;
    g_page_manager.history_depth = 0; // 初始化堆栈为空
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
void Switch_Page(Page_Base* new_page) {
    _Switch_Page_Internal(new_page, true); // true 表示这是一次正常的、需要记录历史的跳转
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
            
            // 这里不用调用 enter，因为已经在切换时调用过了

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

        // 如果起始页面有 loop 函数 (比如它是主时钟页)，就调用它
        if (g_page_manager.page_from && g_page_manager.page_from->loop) {
            g_page_manager.page_from->loop(g_page_manager.page_from);
        }
        // 如果目标页面有 loop 函数 (比如它是主时钟页)，也调用它
        if (g_page_manager.page_to && g_page_manager.page_to->loop) {
            g_page_manager.page_to->loop(g_page_manager.page_to);
        }

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

void Go_Back_Page(void) {
    if (g_page_manager.history_depth > 0) {
        g_page_manager.history_depth--;
        Page_Base* last_page = g_page_manager.history_stack[g_page_manager.history_depth];
        
        // 直接切换到上一个页面，但不将它再次压栈
        // (我们需要一个不带压栈功能的内部切换函数)
        // 让我们为此创建一个内部函数 _Switch_Page_Internal
        _Switch_Page_Internal(last_page, false); // false 表示不记录这次返回操作到历史
    }
}