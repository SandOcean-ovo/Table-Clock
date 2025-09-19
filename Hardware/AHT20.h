/**
 * @file      AHT20.h
 * @brief     温湿度传感器AHT20驱动头文件
 * @details   定义了AHT20传感器的I2C地址、命令以及操作函数原型。
 * @author    SandOcean
 * @date      2025-09-09
 * @version   1.0
 * @copyright Copyright (c) 2025 SandOcean
 */

#ifndef __AHT20_H
#define __AHT20_H

#include "main.h"
#include "stdint.h"
#include "i2c.h" // 确保包含了你的I2C头文件

/**
 * @defgroup AHT20_Driver AHT20温湿度传感器驱动
 * @brief 提供了AHT20的初始化、复位和数据读取功能
 * @{
 */

/** 
 * @defgroup AHT20_Config AHT20 配置参数
 * @{ 
 */
#define AHT20_ADDRESS (0x38 << 1)      ///< AHT20 I2C设备地址 (7位地址左移一位)
/** @} */

/** 
 * @defgroup AHT20_Commands AHT20 命令
 * @{ 
 */
#define AHT20_CMD_INIT      0xBE  ///< 初始化命令
#define AHT20_CMD_TRIGGER   0xAC  ///< 触发测量命令
#define AHT20_CMD_SOFT_RST  0xBA  ///< 软复位命令
/** @} */

/** 
 * @defgroup AHT20_Status_Bits AHT20 状态位
 * @{ 
 */
#define AHT20_STATUS_BUSY   0x80  ///< 状态位：忙 (Busy)
#define AHT20_STATUS_CAL    0x08  ///< 状态位：已校准 (Calibration Enabled)
/** @} */

/**
 * @brief 初始化AHT20传感器
 * @details 检查传感器状态，如果传感器未校准，则向其发送初始化命令。
 * @param[in] hi2c 指向目标I2C外设的HAL句柄指针
 * @return HAL_StatusTypeDef HAL库的I2C操作状态
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
