/**
 * @file DS3231.c
 * @brief DS3231实时时钟芯片驱动实现
 * @details 本文件实现了DS3231实时时钟芯片的完整驱动功能，包括：
 *          - 时间设置和读取
 *          - 温度读取
 *          - 编译时间自动设置
 *          - AT24C32 EEPROM读写操作
 * @author Sandocean
 * @date 2025-08-25
 * @version 1.0
 * @note 本驱动基于STM32 HAL库实现，支持I2C通信
 */

#include "DS3231.h"

// 定义一个静态的I2C句柄指针，用于驱动内部使用
static I2C_HandleTypeDef *ds3231_i2c;

// --- 私有辅助函数 ---

// 十进制转BCD码
static uint8_t decToBcd(int val)
{
    return (uint8_t)((val / 10 * 16) + (val % 10));
}

// BCD码转十进制
static int bcdToDec(uint8_t val)
{
    return (int)((val / 16 * 10) + (val % 16));
}


// --- 公共函数实现 ---

void DS3231_Init(I2C_HandleTypeDef *hi2c)
{
    ds3231_i2c = hi2c;
}

void DS3231_SetTime(Time_t *time)
{
    uint8_t tx_data[7];
    tx_data[0] = decToBcd(time->second);
    tx_data[1] = decToBcd(time->minute);
    tx_data[2] = decToBcd(time->hour);
    tx_data[3] = decToBcd(time->week);
    tx_data[4] = decToBcd(time->day);
    tx_data[5] = decToBcd(time->month);
    tx_data[6] = decToBcd(time->year - 2000); // DS3231年份只存后两位

    // 从寄存器地址0x00开始，连续写入7个字节
    HAL_I2C_Mem_Write(ds3231_i2c, DS3231_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, tx_data, 7, 1000);
}

void DS3231_GetTime(Time_t *time)
{
    uint8_t rx_data[7];
    // 从寄存器地址0x00开始，连续读取7个字节
    HAL_I2C_Mem_Read(ds3231_i2c, DS3231_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, rx_data, 7, 1000);

    time->second = bcdToDec(rx_data[0]);
    time->minute = bcdToDec(rx_data[1]);
    time->hour   = bcdToDec(rx_data[2]);
    time->week   = bcdToDec(rx_data[3]);
    time->day    = bcdToDec(rx_data[4]);
    time->month  = bcdToDec(rx_data[5]);
    time->year   = bcdToDec(rx_data[6]) + 2000;
}

float DS3231_GetTemperature(void)
{
    uint8_t temp_data[2];
    // 读取温度寄存器 0x11 和 0x12
    HAL_I2C_Mem_Read(ds3231_i2c, DS3231_ADDRESS, 0x11, I2C_MEMADD_SIZE_8BIT, temp_data, 2, 1000);
    // 整数部分在temp_data[0]，小数部分在temp_data[1]的高两位 (步进0.25)
    return (float)temp_data[0] + ((temp_data[1] >> 6) * 0.25f);
}


void DS3231_SetTimeFromCompileTime(void)
{
    Time_t t;
    char month_str[4];

    // 1. 定义与 %d 匹配的临时 int 变量
    int day_temp, year_temp;
    int hour_temp, minute_temp, second_temp;
    
    // 2. 使用临时变量的指针传给 sscanf
    sscanf(__DATE__, "%s %d %d", month_str, &day_temp, &year_temp);
    sscanf(__TIME__, "%d:%d:%d", &hour_temp, &minute_temp, &second_temp);

    // 3. 将临时变量的值安全地赋值给结构体
    t.day = (uint8_t)day_temp;
    t.year = (uint16_t)year_temp;
    t.hour = (uint8_t)hour_temp;
    t.minute = (uint8_t)minute_temp;
    t.second = (uint8_t)second_temp;

    // 将月份字符串转换为数字 (这部分逻辑不变)
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for (int i = 0; i < 12; i++)
    {
        if (strncmp(month_str, months[i], 3) == 0) // 使用 strncmp 更安全
        {
            t.month = i + 1;
            break;
        }
    }
    
    // 星期需要自己计算或给个默认值
    t.week = 1; 

    DS3231_SetTime(&t);
}

// --- EEPROM AT24C32 函数实现 ---

HAL_StatusTypeDef AT24C32_WriteByte(uint16_t mem_addr, uint8_t data)
{
    // 关键！AT24C32的内存地址是16位的
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(ds3231_i2c, AT24C32_ADDRESS, mem_addr, I2C_MEMADD_SIZE_16BIT, &data, 1, 1000);
    HAL_Delay(5); // EEPROM写入后需要一个短暂的延时来完成内部操作
    return status;
}

uint8_t AT24C32_ReadByte(uint16_t mem_addr)
{
    uint8_t data;
    HAL_I2C_Mem_Read(ds3231_i2c, AT24C32_ADDRESS, mem_addr, I2C_MEMADD_SIZE_16BIT, &data, 1, 1000);
    return data;
}

HAL_StatusTypeDef AT24C32_WritePage(uint16_t mem_addr, uint8_t *data, uint16_t size)
{
    // AT24C32页大小为32字节，地址范围0x0000-0x0FFF
    const uint16_t PAGE_SIZE = 32;
    const uint16_t MAX_ADDRESS = 0x0FFF;
    
    // 检查地址是否超出范围
    if (mem_addr > MAX_ADDRESS) {
        return HAL_ERROR;
    }
    
    // 计算当前页的起始地址和结束地址
    uint16_t page_start = (mem_addr / PAGE_SIZE) * PAGE_SIZE;
    uint16_t page_end = page_start + PAGE_SIZE - 1;
    
    // 检查是否会跨页
    if (mem_addr + size - 1 > page_end) {
        // 跨页情况：计算当前页可写入的字节数
        uint16_t bytes_in_current_page = page_end - mem_addr + 1;
        
        // 先写入当前页可容纳的数据
        HAL_StatusTypeDef status = HAL_I2C_Mem_Write(ds3231_i2c, AT24C32_ADDRESS, mem_addr, 
                                                    I2C_MEMADD_SIZE_16BIT, data, bytes_in_current_page, 1000);
        if (status != HAL_OK) {
            return status;
        }
        HAL_Delay(5);
        
        // 如果还有剩余数据，写入下一页
        if (bytes_in_current_page < size) {
            uint16_t remaining_bytes = size - bytes_in_current_page;
            uint16_t next_page_addr = page_end + 1;
            
            // 检查下一页地址是否有效
            if (next_page_addr <= MAX_ADDRESS) {
                status = HAL_I2C_Mem_Write(ds3231_i2c, AT24C32_ADDRESS, next_page_addr, 
                                          I2C_MEMADD_SIZE_16BIT, data + bytes_in_current_page, remaining_bytes, 1000);
                HAL_Delay(5);
                return status;
            } else {
                return HAL_ERROR; // 下一页地址超出范围
            }
        }
        
        return HAL_OK;
    } else {
        // 不跨页：直接写入
        if (size > PAGE_SIZE) size = PAGE_SIZE; // 限制单次写入不超过页大小
        HAL_StatusTypeDef status = HAL_I2C_Mem_Write(ds3231_i2c, AT24C32_ADDRESS, mem_addr, 
                                                    I2C_MEMADD_SIZE_16BIT, data, size, 1000);
        HAL_Delay(5);
        return status;
    }
}

HAL_StatusTypeDef AT24C32_ReadPage(uint16_t mem_addr, uint8_t *data, uint16_t size)
{
    return HAL_I2C_Mem_Read(ds3231_i2c, AT24C32_ADDRESS, mem_addr, I2C_MEMADD_SIZE_16BIT, data, size, 1000);
}