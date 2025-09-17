/**
 * @file app_settings.c
 * @brief 应用程序设置管理模块
 * @details 此文件实现了应用程序设置管理功能，包括设置数据的加载、保存和校验和验证。
 * @author SandOcean
 * @date 2025-08-25
 * @version 1.0
 */

#include "app_settings.h"


Settings_t g_app_settings = {
    .magic_number = APP_SETTINGS_MAGIC_NUMBER,
    .language = 0, // 默认语言：中文
    .auto_off = NEVER, // 默认自动关机：关闭
    .dst_enabled = false, // 默认夏令时：关闭
    .checksum = 0
};

uint8_t g_is_screen_off = 0;          

// --- 模块私有函数 ---

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
    AT24C32_WritePage(APP_SETTINGS_ADDRESS, (uint8_t*)settings, sizeof(Settings_t));
}


static void settings_load(Settings_t *settings)
{
    AT24C32_ReadPage(APP_SETTINGS_ADDRESS, (uint8_t*)settings, sizeof(Settings_t));
}

// --- 公共函数实现 ---

void app_settings_init(void)
{
    if(app_settings_load(&g_app_settings) != 0) {
        g_app_settings.magic_number = APP_SETTINGS_MAGIC_NUMBER;
        g_app_settings.language = 0;
        g_app_settings.auto_off = NEVER;
        g_app_settings.checksum = __checksum(&g_app_settings);

        settings_save(&g_app_settings);
    }

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

uint8_t app_settings_save(Settings_t *settings)
{
    Settings_t temp_settings; // 使用临时变量避免修改传入的指针数据

    settings->checksum = __checksum(settings);
    settings_save(settings);
    HAL_Delay(10); // EEPROM写入需要时间

    // 为了确保数据正确写入，再读回来验证一次
    settings_load(&temp_settings);
    if (memcmp(&temp_settings, settings, sizeof(Settings_t)) == 0) {
        return 1;
    }else {
        return 0;
    }

}



