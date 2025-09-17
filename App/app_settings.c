/**
 * @file      app_settings.c
 * @brief     应用程序设置管理模块
 * @details   此文件实现了应用程序设置管理功能，包括设置数据的加载、保存和校验和验证。
 * @author    SandOcean
 * @date      2025-08-25
 * @version   1.0
 * @copyright Copyright (c) 2025 SandOcean
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
    // 校验和应该覆盖所有需要保护的数据成员
    // 从 magic_number 之后开始，一直到 checksum 成员之前
    for (size_t i = offsetof(Settings_t, language); i < offsetof(Settings_t, checksum); i++) {
        sum += p[i];
    }
    return sum;
}

static HAL_StatusTypeDef settings_save(Settings_t *settings)
{
    return AT24C32_WritePage(APP_SETTINGS_ADDRESS, (uint8_t*)settings, sizeof(Settings_t));
}


static HAL_StatusTypeDef settings_load(Settings_t *settings)
{
    return AT24C32_ReadPage(APP_SETTINGS_ADDRESS, (uint8_t*)settings, sizeof(Settings_t));
}

// --- 公共函数实现 ---

bool app_settings_init(void)
{
    if(app_settings_load(&g_app_settings) == false) {
        g_app_settings.magic_number = APP_SETTINGS_MAGIC_NUMBER;
        g_app_settings.language = 0;
        g_app_settings.auto_off = NEVER;
        g_app_settings.checksum = __checksum(&g_app_settings);

        app_settings_save(&g_app_settings);
        return false; 
    }
    else {return true;}

}

bool app_settings_load(Settings_t *settings)
{
    if(settings_load(settings) != HAL_OK) return false; // 先把数据读出来

    // 检查魔法数和校验和
    if (settings->magic_number != APP_SETTINGS_MAGIC_NUMBER) {
        return false; // 魔法数不对，数据无效
    }
    if (settings->checksum != __checksum(settings)) {
        return false; // 校验和不对，数据已损坏
    }
    
    return true; // 数据有效，加载成功
}

bool app_settings_save(Settings_t *settings)
{
    Settings_t temp_to_write = *settings; // 1. 复制一份数据，避免修改原始的 g_app_settings

    // 2. 在副本上计算并设置校验和
    temp_to_write.checksum = __checksum(&temp_to_write);
    
    // 3. 将带有正确校验和的副本写入EEPROM
    settings_save(&temp_to_write);
    HAL_Delay(10); 

    // 4. 为了验证，再读回来到另一个临时变量
    Settings_t temp_read_back;
    settings_load(&temp_read_back);
    
    // 5. 比较写入的副本和读回的数据
    if (memcmp(&temp_to_write, &temp_read_back, sizeof(Settings_t)) == 0) {
        // 验证成功后，才更新全局变量 g_app_settings 的内容
        *settings = temp_to_write;
        return true;
    } else {
        return false;
    }
}



