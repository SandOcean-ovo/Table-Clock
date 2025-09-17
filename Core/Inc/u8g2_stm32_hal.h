/**
 * @file u8g2_stm32_hal.h
 * @brief U8g2图形库与STM32 HAL库的适配层头文件
 * @details 本头文件提供了U8g2图形库与STM32 HAL库之间的接口，包括：
 *          - I2C通信回调函数声明
 *          - GPIO和延时回调函数声明
 *          - U8g2初始化函数声明
 *          - 外部I2C句柄声明
 * @author Sandocean
 * @date 2025-08-25
 * @version 1.0
 */

#ifndef __U8G2_STM32_HAL_H
#define __U8G2_STM32_HAL_H

#include "u8g2.h"
#include "main.h"

extern u8g2_t u8g2; // 全局U8g2实例

/**
 * @brief U8g2的GPIO和延时回调函数
 * @param u8x8 U8g2显示对象指针
 * @param msg 消息类型
 * @param arg_int 整数参数
 * @param arg_ptr 指针参数
 * @return 操作结果，1表示成功，0表示失败
 */
uint8_t u8x8_stm32_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

/**
 * @brief U8g2的I2C通信回调函数
 * @param u8x8 U8g2显示对象指针
 * @param msg 消息类型
 * @param arg_int 整数参数
 * @param arg_ptr 指针参数
 * @return 操作结果，1表示成功，0表示失败
 */
uint8_t u8x8_byte_stm32_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

/**
 * @brief 初始化U8g2显示对象
 * @param u8g2 指向U8g2显示对象的指针
 * @details 配置U8g2对象，设置通信回调和显示参数，适用于SSD1306 OLED显示器
 */
void u8g2Init(u8g2_t *u8g2);


extern I2C_HandleTypeDef hi2c1; 

#endif /* __U8G2_STM32_HAL_H */
