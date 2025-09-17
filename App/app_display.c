/**
 * @file      app_display.c
 * @brief     显示管理器实现文件
 * @details   本文件实现了页面管理器，负责页面的切换、动画、绘制和事件分发。
 *            它维护一个页面历史堆栈，以支持返回上一页的功能。
 * @author    SandOcean
 * @date      2025-09-18
 * @version   1.1
 * @copyright Copyright (c) 2025 SandOcean
 */ 

#include "app_display.h"
#include "u8g2_stm32_hal.h"
#include <string.h>
#include <stdbool.h>
#include "input.h"

/**
 * @defgroup PageManager 页面管理器
 * @{
 */

/* Private defines -----------------------------------------------------------*/
#define PAGE_HISTORY_MAX_DEPTH 8 ///< 页面历史堆栈的最大深度

/* Private types -------------------------------------------------------------*/
/**
 * @brief 页面管理器自身的状态枚举
 */
typedef enum {
    MANAGER_STATE_IDLE,     ///< 静止状态，按页面的刷新率工作
    MANAGER_STATE_ANIMATING ///< 动画状态，以最快速度工作以保证动画流畅
} Manager_State_e;

/* Private variables ---------------------------------------------------------*/
/**
 * @brief 页面管理器内部状态结构体
 */
static struct {
    u8g2_t* u8g2;                   ///< 指向u8g2实例的指针
    Page_Base* current_page;        ///< 指向当前活动页面的指针

    Manager_State_e state;          ///< 管理器当前状态
    Page_Base* page_from;           ///< 动画的来源页面
    Page_Base* page_to;             ///< 动画的目标页面
    uint32_t anim_start_time;       ///< 动画开始的时间戳
    uint32_t anim_duration;         ///< 动画总时长
    
    Page_Base* history_stack[PAGE_HISTORY_MAX_DEPTH]; ///< 存储历史页面的数组
    int8_t history_depth;                             ///< 当前堆栈深度 (或叫栈顶指针)

} g_page_manager;

/* Private function prototypes -----------------------------------------------*/
static void _Switch_Page_Internal(Page_Base* new_page, bool record_history);

/* Function implementations --------------------------------------------------*/

/**
 * @brief  内部页面切换函数
 * @details 处理页面切换的核心逻辑，包括调用退出/进入函数和启动切换动画。
 * @param[in] new_page 要切换到的新页面
 * @param[in] record_history 是否将当前页面记录到历史堆栈中
 * @return 无
 */
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

    if (new_page->enter) {
        new_page->enter(new_page);
    }

    g_page_manager.page_from = g_page_manager.current_page;
    g_page_manager.page_to = new_page;
    g_page_manager.anim_start_time = HAL_GetTick();
    g_page_manager.anim_duration = 250;
    g_page_manager.state = MANAGER_STATE_ANIMATING;
}

/**
 * @brief  初始化页面管理器
 * @details 设置u8g2实例，初始化历史堆栈，并进入指定的初始页面。
 * @param[in] u8g2_ptr 指向已初始化的u8g2实例的指针
 * @return 无
 */
void Page_Manager_Init(u8g2_t* u8g2_ptr) {
    g_page_manager.u8g2 = u8g2_ptr;
    g_page_manager.history_depth = 0;
    g_page_manager.current_page = &g_page_main;
    if (g_page_manager.current_page->enter) {
        g_page_manager.current_page->enter(g_page_manager.current_page);
    }
}

/**
 * @brief  切换到指定的页面
 * @details 这是一个公共接口，用于启动带动画的页面切换，并记录历史。
 * @param[in] new_page 指向目标页面的指针
 * @return 无
 */
void Switch_Page(Page_Base* new_page) {
    _Switch_Page_Internal(new_page, true);
}

/**
 * @brief  页面管理器的主循环函数
 * @details 这是应用主循环中需要调用的核心函数。它根据当前状态处理动画或页面的常规逻辑。
 * @return 无
 */
void Page_Manager_Loop(void)
{
    // 优先处理动画状态
    if (g_page_manager.state == MANAGER_STATE_ANIMATING) {
        uint32_t elapsed = HAL_GetTick() - g_page_manager.anim_start_time;

        // 动画结束
        if (elapsed >= g_page_manager.anim_duration) {
            g_page_manager.state = MANAGER_STATE_IDLE;
            g_page_manager.current_page = g_page_manager.page_to;
            
            // 动画结束后，立即强制刷新一次最终画面
            if (g_page_manager.current_page && g_page_manager.current_page->draw) {
                 u8g2_FirstPage(g_page_manager.u8g2);
                 do {
                     g_page_manager.current_page->draw(g_page_manager.current_page, g_page_manager.u8g2, 0, 0);
                 } while (u8g2_NextPage(g_page_manager.u8g2));
            }
            return;
        }

        // 动画进行中
        float progress = (float)elapsed / g_page_manager.anim_duration;
        int16_t screen_width = u8g2_GetDisplayWidth(g_page_manager.u8g2);
        int16_t from_x = -(int16_t)(screen_width * progress);
        int16_t to_x = screen_width - (int16_t)(screen_width * progress);

        if (g_page_manager.page_from && g_page_manager.page_from->loop) {
            g_page_manager.page_from->loop(g_page_manager.page_from);
        }
        if (g_page_manager.page_to && g_page_manager.page_to->loop) {
            g_page_manager.page_to->loop(g_page_manager.page_to);
        }

        // 同时绘制旧页面和新页面，并施加位移以产生动画
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
    // 如果是静止状态，则按常规逻辑工作
    else { 
        Page_Base* current = g_page_manager.current_page;
        if (!current) return;

        // 处理输入事件
        Input_Event_Data_t event;
        if (input_get_event(&event)) {
            if (current->action) {
                current->action(current, g_page_manager.u8g2, &event);
            }
        }

        // 调用当前页面的循环逻辑
        if (current->loop) {
            current->loop(current);
        }

        // 根据页面的刷新率判断是否需要重绘
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

/**
 * @brief  返回到上一个页面
 * @details 从历史堆栈中弹出上一个页面，并启动切换动画。
 * @return 无
 */
void Go_Back_Page(void) {
    if (g_page_manager.history_depth > 0) {
        g_page_manager.history_depth--;
        Page_Base* last_page = g_page_manager.history_stack[g_page_manager.history_depth];
        
        _Switch_Page_Internal(last_page, false); // false 表示不记录这次返回操作到历史
    }
}

/**
 * @brief  强制返回到主页面
 * @details 清空所有页面历史记录，并立即将当前页面设置为主页面，无切换动画。
 *          主要用于自动熄屏后恢复等场景。
 * @return 无
 */
void Page_Manager_Go_Home(void)
{
    if (g_page_manager.current_page == &g_page_main && g_page_manager.state == MANAGER_STATE_IDLE) {
        return;
    }

    if (g_page_manager.current_page && g_page_manager.current_page->exit) {
        g_page_manager.current_page->exit(g_page_manager.current_page);
    }

    g_page_manager.history_depth = 0;

    g_page_manager.current_page = &g_page_main;
    g_page_manager.state = MANAGER_STATE_IDLE;

    if (g_page_manager.current_page->enter) {
        g_page_manager.current_page->enter(g_page_manager.current_page);
    }
}

/**
 * @}
 */