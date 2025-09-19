/**
 * @file      DS3231.c
 * @brief     DS3231实时时钟芯片驱动实现
 * @details   本文件实现了DS3231实时时钟芯片的完整驱动功能，包括：
 *            - 时间设置和读取
 *            - 温度读取
 *            - 编译时间自动设置
 *            - AT24C32 EEPROM读写操作
 * @author    Sandocean
 * @date      2025-08-25
 * @version   1.0
 * @note      本驱动基于STM32 HAL库实现，支持I2C通信
 * @copyright Copyright (c) 2025 SandOcean
 */

#include "DS3231.h"

/**
 * @addtogroup DS3231_Driver
 * @{
 */

/* Private variables ---------------------------------------------------------*/
/**
 * @brief 用于保存I2C句柄的静态指针
 * @details 在 DS3231_Init 函数中被初始化，供模块内所有函数使用。
 */
static I2C_HandleTypeDef *ds3231_i2c;

/* Private Function implementations ------------------------------------------*/

/**
 * @brief 十进制数转换为BCD码
 * @param[in] val 待转换的十进制数 (0-99)
 * @return uint8_t 转换后的BCD码
 */
static uint8_t decToBcd(uint8_t val)
{
    return (uint8_t)((val / 10 * 16) + (val % 10));
}

/**
 * @brief BCD码转换为十进制数
 * @param[in] val 待转换的BCD码
 * @return uint8_t 转换后的十进制数
 */
static uint8_t bcdToDec(uint8_t val)
{
    return (int)((val / 16 * 10) + (val % 16));
}

/**
 * @brief 获取指定年份和月份的天数
 * @details 考虑了闰年的情况。
 * @param[in] year 年份
 * @param[in] month 月份 (1-12)
 * @return uint8_t 该月的天数
 */
static uint8_t get_days_in_month(uint16_t year, uint8_t month)
{
    if (month == 2) {
        bool is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        return is_leap ? 29 : 28;
    } else if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    } else {
        return 31;
    }
}

/**
 * @brief 判断给定日期是否在夏令时 (DST) 区间内
 * @details 这是一个简化的、基于固定月/日的北半球规则判断。
 *          具体规则由 `DST_START_MONTH`, `DST_START_DAY`, `DST_END_MONTH`, `DST_END_DAY` 宏定义决定。
 * @param[in] time 指向包含当前日期时间的Time_t结构体指针
 * @return bool 是否在夏令时区间内
 *         - @retval true 当前日期在夏令时区间内
 *         - @retval false 当前日期不在夏令时区间内
 */
static bool is_in_dst_period(const Time_t *time)
{
    // 规则：从 DST_START_MONTH 的 DST_START_DAY 到 DST_END_MONTH 的 DST_END_DAY
    
    // 1. 如果当前月份在开始和结束月份之间，则肯定是夏令时
    //    例如，规则是3月到11月，现在是7月
    if (time->month > DST_START_MONTH && time->month < DST_END_MONTH) {
        return true;
    }

    // 2. 如果当前月份是开始月份，则需要检查日期
    //    例如，规则是3月10日开始，现在是3月15日
    if (time->month == DST_START_MONTH && time->day >= DST_START_DAY) {
        return true;
    }

    // 3. 如果当前月份是结束月份，则需要检查日期
    //    例如，规则是11月3日结束，现在是11月1日
    if (time->month == DST_END_MONTH && time->day < DST_END_DAY) {
        return true;
    }
    
    // 4. 其他所有情况都不是夏令时
    return false;
}


/* Public Function implementations -------------------------------------------*/

/**
 * @addtogroup DS3231_Functions
 * @{
 */

/**
 * @brief 初始化DS3231驱动
 * @details 保存I2C句柄以供模块内部其他函数使用。
 * @param[in] hi2c I2C句柄指针
 * @return 无
 */
void DS3231_Init(I2C_HandleTypeDef *hi2c)
{
    ds3231_i2c = hi2c;
}

/**
 * @brief 设置DS3231的时间和日期
 * @details 将 `Time_t` 结构体中的十进制时间转换为BCD码，并通过I2C写入DS3231的寄存器。
 * @param[in] time 指向Time_t结构体的指针，包含要设置的时间信息
 * @return 无
 */
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

/**
 * @brief 从DS3231获取当前时间和日期
 * @details 通过I2C从DS3231寄存器读取BCD码格式的时间，并将其转换为十进制存入 `Time_t` 结构体。
 * @param[out] time 指向Time_t结构体的指针，用于存储读取的时间信息
 * @return 无
 */
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

/**
 * @brief 获取应用了夏令时规则的时间
 * @details 如果夏令时被启用且当前日期在夏令时区间内，
 *          此函数将在标准时间的基础上将小时数加一。
 * @param[out] time 指向Time_t结构体的指针，用于存储最终的时间信息
 * @param[in] dst_enabled 一个布尔值，指示是否应启用夏令时计算
 * @return 无
 */
void DS3231_DST_GetTime(Time_t *time, bool dst_enabled)
{
    // 1. 首先，获取标准的、未经修改的硬件时间
    DS3231_GetTime(time);

    // 2. 如果夏令时功能被禁用了，直接返回标准时间
    if (!dst_enabled) {
        return;
    }

    // 3. 如果夏令时已启用，判断当前日期是否在夏令时区间内
    if (is_in_dst_period(time))
    {
        // 4. 在夏令时区间内，将小时数加一
        time->hour += 1;

        // 5. 处理小时进位（例如，从23:59跳到夏令时的00:59）
        if (time->hour >= 24)
        {
            time->hour = 0; // 小时归零，进入下一天

            // 星期进位 (1=周一, 7=周日)
            time->week++;
            if (time->week > 7) {
                time->week = 1;
            }

            // 日期进位
            time->day++;
            if (time->day > get_days_in_month(time->year, time->month)) {
                time->day = 1; // 日期归1，月份进位
                time->month++;
                if (time->month > 12) {
                    time->month = 1; // 月份归1，年份进位
                    time->year++;
                }
            }
        }
    }
    // 如果不在夏令时区间内，则什么都不做，直接使用标准时间
}

/**
 * @brief 获取DS3231内部温度传感器的温度值
 * @details 读取温度寄存器并根据数据手册公式进行转换。
 * @return float 温度值，单位为摄氏度，精度为0.25°C
 */
float DS3231_GetTemperature(void)
{
    uint8_t temp_data[2];
    // 读取温度寄存器 0x11 和 0x12
    HAL_I2C_Mem_Read(ds3231_i2c, DS3231_ADDRESS, 0x11, I2C_MEMADD_SIZE_8BIT, temp_data, 2, 1000);
    // 整数部分在temp_data[0]，小数部分在temp_data[1]的高两位 (步进0.25)
    return (float)temp_data[0] + ((temp_data[1] >> 6) * 0.25f);
}


/**
 * @brief 从编译时间自动设置DS3231时间
 * @details 此函数在首次烧录或时间需要重置时非常有用，
 *          它会读取编译器在编译时刻的 `__DATE__` 和 `__TIME__` 宏，
 *          解析后将其写入DS3231。
 * @return 无
 */
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

    // 将月份字符串转换为数字 
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for (int i = 0; i < 12; i++)
    {
        if (strncmp(month_str, months[i], 3) == 0) 
        {
            t.month = i + 1;
            break;
        }
    }
    
    // 星期需要自己计算或给个默认值
    t.week = 1; 

    DS3231_SetTime(&t);
}

/** @} */

/**
 * @addtogroup AT24C32_Functions
 * @{
 */

/**
 * @brief 在AT24C32 EEPROM指定地址写入一个字节
 * @param[in] mem_addr 内存地址 (0x0000 - 0x0FFF)
 * @param[in] data 要写入的数据字节
 * @return HAL_StatusTypeDef - HAL库返回的I2C操作状态
 */
HAL_StatusTypeDef AT24C32_WriteByte(uint16_t mem_addr, uint8_t data)
{
    // 关键！AT24C32的内存地址是16位的
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(ds3231_i2c, AT24C32_ADDRESS, mem_addr, I2C_MEMADD_SIZE_16BIT, &data, 1, 1000);
    HAL_Delay(5); // EEPROM写入后需要一个短暂的延时来完成内部操作
    return status;
}

/**
 * @brief 从AT24C32 EEPROM指定地址读取一个字节
 * @param[in] mem_addr 内存地址 (0x0000 - 0x0FFF)
 * @return uint8_t 读取到的数据字节
 */
uint8_t AT24C32_ReadByte(uint16_t mem_addr)
{
    uint8_t data;
    HAL_I2C_Mem_Read(ds3231_i2c, AT24C32_ADDRESS, mem_addr, I2C_MEMADD_SIZE_16BIT, &data, 1, 1000);
    return data;
}

/**
 * @brief 向AT24C32 EEPROM写入一页数据
 * @details 实现了跨页写入的逻辑。如果写入数据超过一页的边界，会自动分块写入。
 * @param[in] mem_addr 起始内存地址 (0x0000 - 0x0FFF)
 * @param[in] data 要写入的数据指针
 * @param[in] size 数据大小
 * @return HAL_StatusTypeDef - HAL库返回的I2C操作状态
 */
HAL_StatusTypeDef AT24C32_WritePage(uint16_t mem_addr, uint8_t *data, uint16_t size)
{
    const uint16_t PAGE_SIZE = 32;
    HAL_StatusTypeDef status = HAL_OK;
    
    uint16_t bytes_remaining = size;
    uint16_t current_addr = mem_addr;
    uint8_t *data_ptr = data;

    // 循环写入，直到所有字节都写入完毕
    while (bytes_remaining > 0)
    {
        // 1. 计算当前页还剩下多少空间
        uint16_t bytes_to_page_end = PAGE_SIZE - (current_addr % PAGE_SIZE);

        // 2. 计算本次实际要写入的字节数
        //    取“剩余字节数”和“到页末尾的字节数”中的较小值
        uint16_t chunk_size = (bytes_remaining < bytes_to_page_end) ? bytes_remaining : bytes_to_page_end;

        // 3. 执行单页内的写入操作
        status = HAL_I2C_Mem_Write(ds3231_i2c, AT24C32_ADDRESS, current_addr, 
                                   I2C_MEMADD_SIZE_16BIT, data_ptr, chunk_size, 1000);
        
        // 4. 如果任何一次写入失败，立即中止并返回错误
        if (status != HAL_OK) {
            return status;
        }

        // 5. 等待EEPROM内部写周期完成 (非常重要)
        HAL_Delay(5);

        // 6. 更新变量，为下一次循环做准备
        bytes_remaining -= chunk_size;
        current_addr += chunk_size;
        data_ptr += chunk_size;
    }

    return HAL_OK;
}


/**
 * @brief 从AT24C32 EEPROM读取一段数据
 * @param[in] mem_addr 起始内存地址 (0x0000 - 0x0FFF)
 * @param[out] data 数据存储缓冲区指针
 * @param[in] size 要读取的数据大小
 * @return HAL_StatusTypeDef - HAL库返回的I2C操作状态
 */
HAL_StatusTypeDef AT24C32_ReadPage(uint16_t mem_addr, uint8_t *data, uint16_t size)
{
    return HAL_I2C_Mem_Read(ds3231_i2c, AT24C32_ADDRESS, mem_addr, I2C_MEMADD_SIZE_16BIT, data, size, 1000);
}

/** 
 * @} 
 */

/**
 * @}
 */
