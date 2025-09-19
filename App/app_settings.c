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
#include <stddef.h> // For offsetof

/**
 * @defgroup AppSettings 应用设置管理
 * @brief 实现了设置的初始化、从EEPROM加载和保存到EEPROM的功能
 * @{
 */

/* Public variables ----------------------------------------------------------*/
/**
 * @brief 全局应用程序设置实例的定义和默认值
 * @details 这是应用设置在内存中的副本。系统启动时会尝试从EEPROM加载，
 *          如果加载失败，则使用这里的默认值。
 */
Settings_t g_app_settings = {
    .magic_number = APP_SETTINGS_MAGIC_NUMBER,
    .language = 0,        ///< 默认语言：中文
    .auto_off = NEVER,    ///< 默认自动关机：关闭
    .dst_enabled = false, ///< 默认夏令时：关闭
    .checksum = 0
};

/**
 * @brief 屏幕是否关闭的标志 (此变量似乎已废弃)
 */
uint8_t g_is_screen_off = 0;          

/* Private function prototypes -----------------------------------------------*/
static uint8_t __checksum(Settings_t *settings);
static HAL_StatusTypeDef settings_save(Settings_t *settings);
static HAL_StatusTypeDef settings_load(Settings_t *settings);

/* Private Function implementations ------------------------------------------*/

/**
 * @brief 计算设置数据的校验和
 * @param[in] settings 指向设置结构体的指针
 * @return uint8_t - 计算出的8位校验和
 */
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

/**
 * @brief 将设置数据写入EEPROM (底层硬件操作)
 * @param[in] settings 指向待写入的设置结构体的指针
 * @return HAL_StatusTypeDef - HAL库返回的I2C操作状态
 */
static HAL_StatusTypeDef settings_save(Settings_t *settings)
{
    return AT24C32_WritePage(APP_SETTINGS_ADDRESS, (uint8_t*)settings, sizeof(Settings_t));
}


/**
 * @brief 从EEPROM读取设置数据 (底层硬件操作)
 * @param[out] settings 指向用于存储读取数据的设置结构体的指针
 * @return HAL_StatusTypeDef - HAL库返回的I2C操作状态
 */
static HAL_StatusTypeDef settings_load(Settings_t *settings)
{
    return AT24C32_ReadPage(APP_SETTINGS_ADDRESS, (uint8_t*)settings, sizeof(Settings_t));
}

/* Public Function implementations -------------------------------------------*/

/**
 * @brief 初始化应用设置
 * @details 尝试从EEPROM加载设置。如果加载失败（数据无效或首次启动），
 *          则将全局设置 `g_app_settings` 初始化为默认值，并将其保存到EEPROM。
 * @return bool 初始化结果
 *         - @retval true 已成功加载现有有效设置。
 *         - @retval false 未找到有效设置，已创建并保存默认设置。
 */
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

/**
 * @brief 从EEPROM加载应用设置
 * @details 从AT24C32 EEPROM中读取设置数据，并进行数据完整性验证：
 *          1. 检查I2C读取是否成功。
 *          2. 检查魔法数是否正确。
 *          3. 验证校验和是否匹配。
 * @param[out] settings 指向设置结构体的指针，用于存储加载的数据
 * @return bool 加载结果
 *         - @retval true 加载成功，数据有效
 *         - @retval false 加载失败，数据无效或已损坏
 */
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

/**
 * @brief 保存应用设置到EEPROM
 * @details 这是一个安全的保存函数，执行以下步骤：
 *          1. 自动计算并更新待保存数据的校验和。
 *          2. 将数据写入EEPROM。
 *          3. 立即从EEPROM读回数据进行验证。
 *          4. 只有在读回的数据与写入的数据完全一致时，才认为保存成功。
 * @param[in,out] settings 指向设置结构体的指针。函数会更新其 `checksum` 成员并保存。
 * @return bool 保存结果
 *         - @retval true 保存并验证成功
 *         - @retval false 保存失败或验证失败
 */
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

/**
 * @}
 */



