/**
 * @file app_settings.c
 * @brief 应用设置函数
 * @details 本文件定义了应用设置函数
 * @author SandOcean
 * @date 2025-08-25
 * @version 1.0
 */

#ifndef __APP_SETTINGS_C
#define __APP_SETTINGS_C

#include "app_settings.h"

#define APP_SETTINGS_MAGIC_NUMBER 0xDEADBEEF

static Settings_t settings;

static uint8_t __checksum(Settings_t *settings)
{
    uint8_t sum = 0;
    uint8_t *p = (uint8_t*)settings;
    // 只计算实际设置项的校验和，不包括magic_number和checksum本身
    for (int i = sizeof(uint32_t); i < sizeof(Settings_t) - 1; i++) {
        sum += p[i];
    }
    return sum;
}

static void settings_save(Settings_t *settings)
{
    AT24C32_WritePage(0, (uint8_t*)&settings, sizeof(Settings_t));
}


static void settings_load(Settings_t *settings)
{
    AT24C32_ReadPage(0, (uint8_t*)&settings, sizeof(Settings_t));
}

void app_settings_init(void)
{
    settings.magic_number = APP_SETTINGS_MAGIC_NUMBER;
    settings.language = 0;
    settings.theme = 0;
    settings.auto_off = 0;
    settings.checksum = __checksum(&settings);
}

uint8_t app_settings_load(Settings_t *settings)
{
    settings_load(settings); // 先把数据读出来

    // 检查魔法数和校验和
    if (settings->magic_number != APP_SETTINGS_MAGIC_NUMBER) {
        return 1; // 魔法数不对，数据无效
    }
    if (settings->checksum != __checksum(settings)) {
        return 1; // 校验和不对，数据已损坏
    }
    
    return 0; // 数据有效，加载成功
}

void app_settings_save(Settings_t *settings)
{
    settings->magic_number != APP_SETTINGS_MAGIC_NUMBER;
    settings->checksum = __checksum(settings);
    settings_save(settings);
}


#endif
