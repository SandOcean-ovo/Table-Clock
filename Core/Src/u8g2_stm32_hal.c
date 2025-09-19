/**
 * @file      u8g2_stm32_hal.c
 * @brief     U8g2图形库与STM32 HAL库的适配层实现
 * @details   本文件实现了U8g2图形库与STM32 HAL库之间的接口，包括：
 *            - I2C通信回调函数实现
 *            - GPIO和延时回调函数实现
 *            - U8g2初始化函数实现
 * @author    Sandocean
 * @date      2025-08-25
 * @version   1.0
 * @note      本适配层专为STM32 HAL库设计，支持I2C通信的OLED显示器
 * @copyright Copyright (c) 2025 SandOcean
 */

#include "u8g2_stm32_hal.h"

/**
 * @defgroup U8g2_HAL_Implementation U8g2 STM32 HAL Implementation
 * @brief U8g2 库与 STM32 HAL 库的适配层函数实现
 * @{
 */

/**
 * @brief 全局U8g2实例
 */
u8g2_t u8g2;

/**
 * @brief U8g2的I2C通信回调函数
 * @details U8g2库通过此函数将I2C通信请求委托给STM32 HAL库。
 *          它处理字节发送、传输开始/结束等消息。
 * @param[in] u8x8 U8g2显示对象指针
 * @param[in] msg U8g2传递的消息类型 (e.g., U8X8_MSG_BYTE_SEND)
 * @param[in] arg_int 消息相关的整数参数
 * @param[in] arg_ptr 消息相关的指针参数 (e.g., a data buffer)
 * @return uint8_t 操作结果
 *         - @retval 1 成功
 *         - @retval 0 失败
 */
uint8_t u8x8_byte_stm32_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    static uint8_t buffer[32]; // 传输缓冲区
    static uint8_t buf_idx;    // 缓冲区索引
    uint8_t *data;

    switch (msg)
    {
    case U8X8_MSG_BYTE_SEND:
        data = (uint8_t *)arg_ptr;
        while (arg_int > 0)
        {
            buffer[buf_idx++] = *data;
            data++;
            arg_int--;
        }
        break;
    case U8X8_MSG_BYTE_INIT:
        // I2C已由CubeMX初始化，此处无需操作
        break;
    case U8X8_MSG_BYTE_SET_DC:
        // 对于I2C，DC线（数据/命令选择）通常由I2C协议本身处理，此处无需操作
        break;
    case U8X8_MSG_BYTE_START_TRANSFER:
        buf_idx = 0;
        break;
    case U8X8_MSG_BYTE_END_TRANSFER:
        // 关键! HAL库的I2C地址是7位的，而U8g2传入的是8位的（包含R/W位），需要右移一位
        if (HAL_I2C_Master_Transmit(&hi2c1, (u8x8->i2c_address), buffer, buf_idx, 1000) != HAL_OK)
        {
            return 0; // 传输失败
        }
        break;
    default:
        return 0;
    }
    return 1;
}

/**
 * @brief U8g2的GPIO和延时回调函数
 * @details U8g2库通过此函数请求GPIO操作（在此I2C实现中未使用）和延时。
 *          它处理毫秒、微秒和纳秒级别的延时请求。
 * @param[in] u8x8 U8g2显示对象指针
 * @param[in] msg U8g2传递的消息类型 (e.g., U8X8_MSG_DELAY_MILLI)
 * @param[in] arg_int 消息相关的整数参数 (e.g., delay duration)
 * @param[in] arg_ptr 消息相关的指针参数 (未使用)
 * @return uint8_t 操作结果
 *         - @retval 1 成功
 *         - @retval 0 失败 (消息不被支持时)
 */
uint8_t u8x8_stm32_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg)
    {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
        // GPIO已由CubeMX初始化，此处无需操作
        break;
    case U8X8_MSG_DELAY_MILLI:
        HAL_Delay(arg_int);
        break;
    case U8X8_MSG_DELAY_10MICRO:
        // 实现一个粗略的10us延时
        for (uint16_t n = 0; n < 320; n++)
        {
            __NOP();
        }
        break;
    case U8X8_MSG_DELAY_NANO:
        // 实现一个粗略的纳秒级延时
        __NOP();
        break;
    // 以下GPIO操作对于纯I2C模式的SSD1306不是必需的，但最好保留
    case U8X8_MSG_GPIO_CS:
        // 片选，I2C模式下不需要
        break;
    case U8X8_MSG_GPIO_DC:
        // 数据/命令选择，I2C模式下不需要
        break;
    case U8X8_MSG_GPIO_RESET:
        // 如果你的OLED有RESET引脚并且你连接了它
        // HAL_GPIO_WritePin(OLED_RES_GPIO_Port, OLED_RES_Pin, arg_int);
        break;
    default:
        return 0;
    }
    return 1;
}

/**
 * @brief 初始化U8g2显示对象
 * @details 此函数执行U8g2的完整初始化流程：
 *          1. 调用 `u8g2_Setup_ssd1306_i2c_128x64_noname_f` 设置显示驱动和回调。
 *          2. 设置显示器的I2C地址。
 *          3. 调用 `u8g2_InitDisplay` 初始化显示控制器。
 *          4. 调用 `u8g2_SetPowerSave(0)` 唤醒显示器。
 *          5. 清空屏幕缓冲区并发送到屏幕。
 * @param[out] u8g2 指向待初始化的U8g2显示对象的指针
 * @return 无
 */
void u8g2Init(u8g2_t *u8g2)
{
    HAL_Delay(150);                                                                                           // 确保显示器上电稳定
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(u8g2, U8G2_R0, u8x8_byte_stm32_hw_i2c, u8x8_stm32_gpio_and_delay); // 初始化 u8g2 结构体
    u8g2_SetI2CAddress(u8g2, 0x78);                                                                           // 设置I2C地址
    u8g2_InitDisplay(u8g2);                                                                                   // 根据所选的芯片进行初始化工作，初始化完成后，显示器处于关闭状态
    u8g2_SetPowerSave(u8g2, 0);                                                                               // 打开显示器
    u8g2_ClearBuffer(u8g2);
    u8g2_SendBuffer(u8g2);
}

/**
 * @}
 */