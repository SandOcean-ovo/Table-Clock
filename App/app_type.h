/**
 * @file      app_type.h
 * @brief     应用类型头文件
 * @details   本文件定义了应用层使用的数据结构
 * @author    SandOcean
 * @date      2025-08-25
 * @version   1.0
 * @copyright Copyright (c) 2025 SandOcean
 */

#ifndef __APP_TYPE_H
#define __APP_TYPE_H

#include "main.h"
#include "input.h"
#include "u8g2.h"
#include "stdint.h"
#include <stdbool.h>

/**
 * @brief 自动熄屏时间枚举
 * @details 定义了自动熄屏设置中可选的时间间隔。
 */
typedef enum {
    NEVER = 0,      ///< 从不自动熄屏
    TIME_30S,       ///< 30秒后自动熄屏
    TIME_1MIN,      ///< 1分钟后自动熄屏
    TIME_5MIN,      ///< 5分钟后自动熄屏
    TIME_10MIN,     ///< 10分钟后自动熄屏
} Auto_Off_e;

/**
 * @brief 应用设置结构体
 * @details 定义了需要持久化保存到EEPROM的所有设置项。
 */
typedef struct {
    uint32_t magic_number;  ///< 魔法数，用于验证EEPROM中的数据是否有效 (固定为 `APP_SETTINGS_MAGIC_NUMBER`)
    uint8_t language;       ///< 语言设置 (0: English, 1: Chinese)
    Auto_Off_e auto_off;    ///< 自动熄屏设置，使用 `Auto_Off_e` 枚举
    bool dst_enabled;       ///< 夏令时 (Daylight Saving Time) 是否启用

    uint8_t checksum;       ///< 校验和，用于验证数据完整性
} Settings_t;


#endif /* __APP_TYPE_H */
