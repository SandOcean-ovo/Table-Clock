/**
 * @file      app_settings.h
 * @brief     应用设置头文件
 * @author    SandOcean
 * @date      2025-08-25
 * @version   1.0
 * @copyright Copyright (c) 2025 SandOcean
 */

#ifndef __APP_SETTINGS_H
#define __APP_SETTINGS_H

#include "main.h"
#include "app_type.h"
#include "app_config.h"
#include "DS3231.h"
#include "stdint.h"
#include <stdbool.h>

#define APP_SETTINGS_ADDRESS      0x0000      ///< 设置信息在AT24C32中存储的起始地址
#define APP_SETTINGS_MAGIC_NUMBER 0xDEADBEEF  ///< 设置数据的魔法数，用于验证数据有效性

/**
 * @brief 全局应用程序设置实例
 * @details 该变量在内存中维护当前的应用设置。
 *          通过调用 `app_settings_load()` 从EEPROM加载，
 *          并通过 `app_settings_save()` 保存回EEPROM。
 */
extern Settings_t g_app_settings;

/**
 * @brief 初始化应用设置
 * @details 尝试从EEPROM加载现有设置。如果加载失败（例如首次启动或数据损坏），
 *          则会创建一套默认设置并保存。
 * @return bool 初始化结果
 *         - @retval true 已成功加载现有有效设置。
 *         - @retval false 未找到有效设置，已创建并保存默认设置。
 */
bool app_settings_init(void);

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
 * @return bool 保存结果
 *         - @retval true 保存成功
 *         - @retval false 保存失败（例如EEPROM写入错误）
 */
bool app_settings_save(Settings_t *settings);

/**
 * @brief 从EEPROM加载应用设置
 * @details 从AT24C32 EEPROM中读取设置数据，并进行数据完整性验证：
 *          - 检查魔法数是否正确
 *          - 验证校验和是否匹配
 * @param[out] settings 指向设置结构体的指针，用于存储加载的数据
 * @return bool 加载结果
 *         - @retval true 加载成功，数据有效
 *         - @retval false 加载失败，数据无效或已损坏
 * @note 如果加载失败，建议调用 `app_settings_init()` 和 `app_settings_save()` 来创建一套新的默认设置。
 */
bool app_settings_load(Settings_t *settings);

#endif /* __APP_SETTINGS_H */
