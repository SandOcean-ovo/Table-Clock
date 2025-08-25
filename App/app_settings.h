/**
 * @file app_settings.h
 * @brief 应用设置函数
 * @author SandOcean
 * @date 2025-08-25
 * @version 1.0
 */

#ifndef __APP_SETTINGS_H
#define __APP_SETTINGS_H

#include "main.h"
#include "app_type.h"
#include "app_config.h"
#include "DS3231.h"
#include "stdint.h"

/**
 * @brief 初始化应用设置
 * @details 初始化默认设置值，包括语言、主题、自动关机等参数。
 *          设置默认值：中文(0)、经典主题(0)、关闭自动关机(0)
 * @note 此函数应在系统启动时调用一次
 * @return None
 */
void app_settings_init(void);

/**
 * @brief 保存应用设置到EEPROM
 * @details 将设置数据保存到AT24C32 EEPROM中，包括：
 *          - 魔法数验证
 *          - 语言设置
 *          - 主题设置  
 *          - 自动关机设置
 *          - 校验和
 * @param[in,out] settings 指向设置结构体的指针
 * @note 保存前会自动计算并更新校验和
 * @return None
 */
void app_settings_save(Settings_t *settings);

/**
 * @brief 从EEPROM加载应用设置
 * @details 从AT24C32 EEPROM中读取设置数据，并进行数据完整性验证：
 *          - 检查魔法数是否正确
 *          - 验证校验和是否匹配
 * @param[out] settings 指向设置结构体的指针，用于存储加载的数据
 * @return uint8_t 加载结果
 *          - @retval 0 加载成功，数据有效
 *          - @retval 1 加载失败，数据无效或已损坏
 * @note 如果加载失败，建议调用app_settings_init()重新初始化默认设置
 */
uint8_t app_settings_load(Settings_t *settings);












#endif /* __APP_SETTINGS_H */
