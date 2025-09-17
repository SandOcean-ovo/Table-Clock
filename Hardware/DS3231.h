/**
 * @file DS3231.h
 * @brief DS3231实时时钟芯片驱动头文件
 * @details 本头文件定义了DS3231实时时钟芯片驱动的接口，包括：
 *          - 时间结构体定义
 *          - 时间设置和读取函数声明
 *          - 温度读取函数声明
 *          - 编译时间自动设置函数声明
 *          - AT24C32 EEPROM读写函数声明
 * @author Sandocean
 * @date 2025-08-25
 * @version 1.0
 * @note 本驱动基于STM32 HAL库实现，支持I2C通信
 */

#ifndef __DS3231_H
#define __DS3231_H

#include "main.h"
#include "i2c.h"
#include "stdint.h"

#define DS3231_ADDRESS 0xD0

#define AT24C32_ADDRESS (0x57 << 1)

/**
 * @brief 时间结构体定义
 * @details 用于存储年、月、日、时、分、秒、星期等时间信息
 */
typedef struct {
    uint8_t hour;    ///< 小时 (0-23)
    uint8_t minute;  ///< 分钟 (0-59)
    uint8_t second;  ///< 秒 (0-59)
    uint16_t year;   ///< 年份 (2000-2099)
    uint8_t month;   ///< 月份 (1-12)
    uint8_t day;     ///< 日期 (1-31)
    uint8_t week;    ///< 星期 (1-7)
} Time_t;

// --- 公共函数声明 ---

/**
 * @brief 初始化DS3231驱动
 * @param hi2c I2C句柄指针
 */
void DS3231_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief 设置DS3231的时间和日期
 * @param time 指向Time_t结构体的指针，包含要设置的时间信息
 */
void DS3231_SetTime(Time_t *time);

/**
 * @brief 从DS3231获取当前时间和日期
 * @param time 指向Time_t结构体的指针，用于存储读取的时间信息
 */
void DS3231_GetTime(Time_t *time);

/**
 * @brief 获取DS3231内部温度传感器的温度值
 * @return 温度值，单位为摄氏度，精度为0.25°C
 */
float DS3231_GetTemperature(void);

/**
 * @brief 从编译时间自动设置DS3231时间
 */
void DS3231_SetTimeFromCompileTime(void);

// --- EEPROM AT24C32 函数 ---

/**
 * @brief 在AT24C32 EEPROM指定地址写入一个字节
 * @param mem_addr 内存地址 (0x0000-0x0FFF)
 * @param data 要写入的数据字节
 * @return HAL_StatusTypeDef 操作状态
 */
HAL_StatusTypeDef AT24C32_WriteByte(uint16_t mem_addr, uint8_t data);

/**
 * @brief 从AT24C32 EEPROM指定地址读取一个字节
 * @param mem_addr 内存地址 (0x0000-0x0FFF)
 * @return 读取到的数据字节
 */
uint8_t AT24C32_ReadByte(uint16_t mem_addr);

/**
 * @brief 向AT24C32 EEPROM写入一页数据
 * @param mem_addr 起始内存地址 (0x0000-0x0FFF)
 * @param data 要写入的数据指针
 * @param size 数据大小，最大32字节
 * @return HAL_StatusTypeDef 操作状态
 * @note 单次写入不能超过32字节且不能跨页
 */
HAL_StatusTypeDef AT24C32_WritePage(uint16_t mem_addr, uint8_t *data, uint16_t size);

/**
 * @brief 从AT24C32 EEPROM读取一段数据
 * @param mem_addr 起始内存地址 (0x0000-0x0FFF)
 * @param data 数据存储缓冲区指针
 * @param size 要读取的数据大小
 * @return HAL_StatusTypeDef 操作状态
 */
HAL_StatusTypeDef AT24C32_ReadPage(uint16_t mem_addr, uint8_t *data, uint16_t size);

#endif /* __DS3231_H */
