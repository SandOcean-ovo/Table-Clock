/**
 * @file      AHT20.c
 * @brief     温湿度传感器AHT20驱动文件
 * @details   本文件实现了AHT20的各种操作，包括初始化、复位和数据读取。
 * @author    SandOcean
 * @date      2025-09-09
 * @version   1.0
 * @copyright Copyright (c) 2025 SandOcean
 */

#include "AHT20.h"

/**
 * @addtogroup AHT20_Driver
 * @{
 */

/* Private variables ---------------------------------------------------------*/
/**
 * @brief 用于保存I2C句柄的静态指针
 * @details 在 AHT20_Init 函数中被初始化，供模块内所有私有函数使用，
 *          避免了在每个函数调用中都传递 I2C 句柄。
 */
static I2C_HandleTypeDef *g_aht20_hi2c = NULL;

/* Private Function implementations ------------------------------------------*/

/**
 * @brief 向AHT20发送命令
 * @param[in] cmd 要发送的命令字节 (e.g., AHT20_CMD_TRIGGER)
 * @param[in] p_data 指向命令参数数据缓冲区的指针 (可以为NULL)
 * @param[in] size 参数数据的长度 (字节)
 * @return HAL_StatusTypeDef - HAL库返回的I2C操作状态
 */
static HAL_StatusTypeDef AHT20_Send_Cmd(uint8_t cmd, uint8_t *p_data, uint16_t size)
{
    uint8_t tx_buffer[3];
    tx_buffer[0] = cmd;
    if (size > 0 && p_data != NULL) {
        tx_buffer[1] = p_data[0];
        tx_buffer[2] = p_data[1];
    }
    return HAL_I2C_Master_Transmit(g_aht20_hi2c, AHT20_ADDRESS, tx_buffer, size + 1, HAL_MAX_DELAY);
}

/**
 * @brief 从AHT20读取状态字节
 * @param[out] p_status 指向用于存储状态字节的变量指针
 * @return HAL_StatusTypeDef - HAL库返回的I2C操作状态
 */
static HAL_StatusTypeDef AHT20_Read_Status(uint8_t *p_status)
{
    return HAL_I2C_Master_Receive(g_aht20_hi2c, AHT20_ADDRESS, p_status, 1, HAL_MAX_DELAY);
}

/* Public Function implementations -------------------------------------------*/

/**
 * @brief 初始化AHT20传感器
 * @details 检查传感器状态，如果传感器未校准，则向其发送初始化命令。
 * @param[in] hi2c 指向目标I2C外设的HAL句柄指针
 * @return HAL_StatusTypeDef HAL库的I2C操作状态
 */
HAL_StatusTypeDef AHT20_Init(I2C_HandleTypeDef *hi2c)
{
    g_aht20_hi2c = hi2c;
    uint8_t status = 0;
    HAL_StatusTypeDef ret;

    HAL_Delay(40); // 上电后延时

    ret = AHT20_Read_Status(&status);
    if (ret != HAL_OK) {
        return ret;
    }

    // 检查校准状态位，如果未校准，则发送初始化命令
    if ((status & AHT20_STATUS_CAL) == 0) {
        uint8_t init_params[2] = {0x08, 0x00};
        ret = AHT20_Send_Cmd(AHT20_CMD_INIT, init_params, 2);
        HAL_Delay(300); // 初始化后需要延时
    }
    
    return ret;
}

/**
 * @brief 软复位AHT20传感器
 * @return HAL_StatusTypeDef HAL状态码
 */
HAL_StatusTypeDef AHT20_Soft_Reset(void)
{
    HAL_StatusTypeDef ret = AHT20_Send_Cmd(AHT20_CMD_SOFT_RST, NULL, 0);
    HAL_Delay(20); // 软复位后需要延时
    return ret;
}

/**
 * @brief 读取AHT20的温度和湿度值
 * @param[out] temperature 指向浮点数的指针，用于存储温度值(℃)
 * @param[out] humidity 指向浮点数的指针，用于存储相对湿度值(%RH)
 * @return HAL_StatusTypeDef HAL状态码
 * @note 此函数会触发一次新的测量，等待测量完成，然后读取并转换数据。
 */
HAL_StatusTypeDef AHT20_Read_Temp_Humi(float *temperature, float *humidity)
{
    if (g_aht20_hi2c == NULL) {
        return HAL_ERROR; // 未初始化
    }

    uint8_t trigger_params[2] = {0x33, 0x00};
    uint8_t read_buffer[6] = {0};
    uint32_t raw_humi = 0;
    uint32_t raw_temp = 0;
    HAL_StatusTypeDef ret;

    // 1. 发送触发测量命令
    ret = AHT20_Send_Cmd(AHT20_CMD_TRIGGER, trigger_params, 2);
    if (ret != HAL_OK) {
        return ret;
    }

    // 2. 延时等待测量完成 (官方建议>75ms)
    HAL_Delay(80);

    // 3. 循环读取状态，直到传感器不忙
    do {
        ret = HAL_I2C_Master_Receive(g_aht20_hi2c, AHT20_ADDRESS, read_buffer, 6, HAL_MAX_DELAY);
        if (ret != HAL_OK) {
            return ret;
        }
        HAL_Delay(5); // 短暂延时避免频繁查询
    } while ((read_buffer[0] & AHT20_STATUS_BUSY) != 0);

    // 4. 转换数据
    // 湿度数据转换
    raw_humi = ((uint32_t)read_buffer[1] << 12) | ((uint32_t)read_buffer[2] << 4) | (read_buffer[3] >> 4);
    *humidity = (float)raw_humi * 100.0f / 1048576.0f; // 1048576 = 2^20

    // 温度数据转换
    raw_temp = (((uint32_t)read_buffer[3] & 0x0F) << 16) | ((uint32_t)read_buffer[4] << 8) | read_buffer[5];
    *temperature = (float)raw_temp * 200.0f / 1048576.0f - 50.0f;

    return HAL_OK;
}

/**
 * @}
 */