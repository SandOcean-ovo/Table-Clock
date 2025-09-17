/**
 * @file app_main.c
 * @brief 应用主函数
 * @details 本文件定义了应用的主函数
 * @author SandOcean
 * @date 2025-09-17
 * @version 1.0
 */

#include "app_main.h"

void app_main_init(void)
{
    app_settings_init();
	u8g2Init(&u8g2);
    DS3231_Init(&hi2c1);
    AHT20_Init(&hi2c1);
    Page_Manager_Init(&u8g2);
    input_init(&htim3, &htim2);
}
void app_main_loop(void)
{
    Page_Manager_Loop();
}
