/**
 * @file      DS3231.h
 * @brief     DS3231实时时钟芯片驱动头文件
 * @details   本头文件定义了DS3231实时时钟芯片驱动的接口，包括：
 *            - 时间结构体定义
 *            - 时间设置和读取函数声明
 *            - 温度读取函数声明
 *            - 编译时间自动设置函数声明
 *            - AT24C32 EEPROM读写函数声明
 * @author    Sandocean
 * @date      2025-08-25
 * @version   1.0
 * @note      本驱动基于STM32 HAL库实现，支持I2C通信
 * @copyright Copyright (c) 2025 SandOcean
 */

#ifndef __DS3231_H
#define __DS3231_H

#include "main.h"
#include "i2c.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @defgroup DS3231_Driver DS3231 RTC 驱动
 * @brief 提供了对DS3231实时时钟的控制，以及对其附带的AT24C32 EEPROM的读写功能。
 * @{
 */

/** 
 * @defgroup DS3231_Config DS3231 配置参数
 * @{ 
 */
#define DS3231_ADDRESS (0x68 << 1)       ///< DS3231 I2C设备地址 (7位地址左移一位)
#define AT24C32_ADDRESS (0x57 << 1)      ///< AT24C32 I2C设备地址 (7位地址左移一位)
/** @} */

/** 
 * @defgroup DST_Config 夏令时配置
 * @brief 定义了夏令时的起止日期 (此为示例，具体规则可能因地区而异)
 * @{ 
 */
#define DST_START_MONTH 3   ///< 夏令时开始月份 (3月)
#define DST_START_DAY   10  ///< 夏令时开始日期 (10日)
#define DST_END_MONTH   11  ///< 夏令时结束月份 (11月)
#define DST_END_DAY     3   ///< 夏令时结束日期 (3日)
/** @} */

/**
 * @brief 时间结构体定义
 * @details 用于存储和传递年、月、日、时、分、秒、星期等时间信息
 */
typedef struct {
    uint8_t hour;    ///< 小时 (0-23)
    uint8_t minute;  ///< 分钟 (0-59)
    uint8_t second;  ///< 秒 (0-59)
    uint16_t year;   ///< 年份 (2000-2099)
    uint8_t month;   ///< 月份 (1-12)
    uint8_t day;     ///< 日期 (1-31)
    uint8_t week;    ///< 星期 (1=周一, 7=周日)
} Time_t;

/** 
 * @defgroup DS3231_Functions DS3231核心功能函数
 * @{ 
 */

/**
 * @brief 初始化DS3231驱动
 * @param[in] hi2c I2C句柄指针
 * @return 无
 */
void DS3231_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief 设置DS3231的时间和日期
 * @param[in] time 指向Time_t结构体的指针，包含要设置的时间信息
 * @return 无
 */
void DS3231_SetTime(Time_t *time);

/**
 * @brief 从DS3231获取当前时间和日期
 * @param[out] time 指向Time_t结构体的指针，用于存储读取的时间信息
 * @return 无
 */
void DS3231_GetTime(Time_t *time);

/**
 * @brief 获取应用了夏令时规则的时间
 * @details 如果夏令时被启用且当前日期在夏令时区间内，
 *          此函数将在标准时间的基础上将小时数加一。
 * @param[out] time 指向Time_t结构体的指针，用于存储最终的时间信息
 * @param[in] dst_enabled 一个布尔值，指示是否应启用夏令时计算
 * @return 无
 */
void DS3231_DST_GetTime(Time_t *time, bool dst_enabled);

/**
 * @brief 获取DS3231内部温度传感器的温度值
 * @return float 温度值，单位为摄氏度，精度为0.25°C
 */
float DS3231_GetTemperature(void);

/**
 * @brief 从编译时间自动设置DS3231时间
 * @details 此函数在首次烧录或时间需要重置时非常有用，
 *          它会读取编译器在编译时刻的系统时间，并将其写入DS3231。
 * @return 无
 */
void DS3231_SetTimeFromCompileTime(void);

/** @} */

/** 
 * @defgroup AT24C32_Functions AT24C32 EEPROM 读写函数
 * @{ 
 */

/**
 * @brief 在AT24C32 EEPROM指定地址写入一个字节
 * @param[in] mem_addr 内存地址 (0x0000 - 0x0FFF)
 * @param[in] data 要写入的数据字节
 * @return HAL_StatusTypeDef - HAL库返回的I2C操作状态
 */
HAL_StatusTypeDef AT24C32_WriteByte(uint16_t mem_addr, uint8_t data);

/**
 * @brief 从AT24C32 EEPROM指定地址读取一个字节
 * @param[in] mem_addr 内存地址 (0x0000 - 0x0FFF)
 * @return uint8_t 读取到的数据字节
 */
uint8_t AT24C32_ReadByte(uint16_t mem_addr);

/**
 * @brief 向AT24C32 EEPROM写入一页数据
 * @param[in] mem_addr 起始内存地址 (0x0000 - 0x0FFF)
 * @param[in] data 要写入的数据指针
 * @param[in] size 数据大小，最大32字节
 * @return HAL_StatusTypeDef - HAL库返回的I2C操作状态
 * @note 单次写入不能超过32字节且不能跨页 (e.g. from 0x1F to 0x20)。
 */
HAL_StatusTypeDef AT24C32_WritePage(uint16_t mem_addr, uint8_t *data, uint16_t size);

/**
 * @brief 从AT24C32 EEPROM读取一段数据
 * @param[in] mem_addr 起始内存地址 (0x0000 - 0x0FFF)
 * @param[out] data 数据存储缓冲区指针
 * @param[in] size 要读取的数据大小
 * @return HAL_StatusTypeDef - HAL库返回的I2C操作状态
 */
HAL_StatusTypeDef AT24C32_ReadPage(uint16_t mem_addr, uint8_t *data, uint16_t size);

/** 
 * @} 
 */

/**
 * @}
 */

#endif /* __DS3231_H */
