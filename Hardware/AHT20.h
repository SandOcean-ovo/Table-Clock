/**
 * @file AHT20.h
 * @brief 温湿度传感器AHT20驱动头文件
 * @details 定义了AHT20传感器的I2C地址、命令以及操作函数原型。
 * @author SandOcean
 * @date 2025-09-09
 * @version 1.0
 */

#ifndef __AHT20_H
#define __AHT20_H

#include "main.h"
#include "stdint.h"
#include "i2c.h" // 确保包含了你的I2C头文件

// AHT20 I2C地址 (7位地址)
#define AHT20_ADDRESS (0x38)

// AHT20 命令
#define AHT20_CMD_INIT      0xBE // 初始化命令
#define AHT20_CMD_TRIGGER   0xAC // 触发测量命令
#define AHT20_CMD_SOFT_RST  0xBA // 软复位命令

// AHT20 状态位
#define AHT20_STATUS_BUSY   0x80 // 状态位：忙
#define AHT20_STATUS_CAL    0x08 // 状态位：已校准

/**
 * @brief 初始化AHT20传感器
 * @param[in] hi2c 指向I2C句柄的指针
 * @return HAL_StatusTypeDef HAL状态码 (HAL_OK, HAL_ERROR, etc.)
 * @note 检查传感器状态，如果未校准则执行初始化。
 */
HAL_StatusTypeDef AHT20_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief 软复位AHT20传感器
 * @return HAL_StatusTypeDef HAL状态码
 */
HAL_StatusTypeDef AHT20_Soft_Reset(void);

/**
 * @brief 读取AHT20的温度和湿度值
 * @param[out] temperature 指向浮点数的指针，用于存储温度值(℃)
 * @param[out] humidity 指向浮点数的指针，用于存储相对湿度值(%RH)
 * @return HAL_StatusTypeDef HAL状态码
 * @note 此函数会触发一次新的测量，等待测量完成，然后读取并转换数据。
 */
HAL_StatusTypeDef AHT20_Read_Temp_Humi(float *temperature, float *humidity);

#endif /* __AHT20_H */