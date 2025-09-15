/**
 * @file AHT20.c
 * @brief 温湿度传感器AHT20驱动文件
 * @details 本文件实现了AHT20的各种操作，包括初始化、复位和数据读取。
 * @author SandOcean
 * @date 2025-09-09
 * @version 1.0
 */

#include "AHT20.h"

// 用于保存I2C句柄的静态指针
static I2C_HandleTypeDef *g_aht20_hi2c = NULL;

// --- 模块私有函数 ---

/**
 * @brief 向AHT20发送命令
 * @param[in] cmd 要发送的命令
 * @param[in] p_data 指向命令参数的指针
 * @param[in] size 参数的长度
 * @return HAL_StatusTypeDef HAL状态码
 */
static HAL_StatusTypeDef AHT20_Send_Cmd(uint8_t cmd, uint8_t *p_data, uint16_t size)
{
    uint8_t tx_buffer[3];
    tx_buffer[0] = cmd;
    if (size > 0 && p_data != NULL) {
        tx_buffer[1] = p_data[0];
        tx_buffer[2] = p_data[1];
    }
    return HAL_I2C_Master_Transmit(g_aht20_hi2c, (AHT20_ADDRESS << 1), tx_buffer, size + 1, HAL_MAX_DELAY);
}

/**
 * @brief 从AHT20读取状态字节
 * @param[out] p_status 指向用于存储状态字节的变量指针
 * @return HAL_StatusTypeDef HAL状态码
 */
static HAL_StatusTypeDef AHT20_Read_Status(uint8_t *p_status)
{
    return HAL_I2C_Master_Receive(g_aht20_hi2c, (AHT20_ADDRESS << 1) | 0x01, p_status, 1, HAL_MAX_DELAY);
}

// --- 公共函数实现 ---

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

HAL_StatusTypeDef AHT20_Soft_Reset(void)
{
    HAL_StatusTypeDef ret = AHT20_Send_Cmd(AHT20_CMD_SOFT_RST, NULL, 0);
    HAL_Delay(20); // 软复位后需要延时
    return ret;
}

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
        ret = HAL_I2C_Master_Receive(g_aht20_hi2c, (AHT20_ADDRESS << 1) | 0x01, read_buffer, 6, HAL_MAX_DELAY);
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