/**
 * @file      app_config.h
 * @brief     配置头文件
 * @details   本文件定义了项目的配置参数，包含应用信息和语言选项。
 * @author    SandOcean
 * @date      2025-08-25
 * @version   1.0
 * @copyright Copyright (c) 2025 SandOcean
 */ 

#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#include "main.h"

/** 
 * @defgroup App_Info 应用信息
 * @brief 定义了应用的版本、作者等静态信息
 * @{ 
 */
#define APP_NAME "TableClock"               ///< 应用名称
#define APP_AUTHOR "SandOcean."              ///< 作者名称
#define APP_COPYRIGHT "Copyright (c) 2025"   ///< 版权信息
#define APP_VERSION_MAJOR "1"                ///< 主版本号
#define APP_VERSION_MINOR "0"                ///< 次版本号
#define APP_VERSION_PATCH "0"                ///< 修订号
#define APP_VERSION APP_VERSION_MAJOR "." APP_VERSION_MINOR "." APP_VERSION_PATCH ///< 完整版本字符串
/** @} */

/** 
 * @defgroup Language_Codes 语言代码
 * @brief 定义了支持的语言代码
 * @{ 
 */
#define LANGUAGE_EN 0 ///< 英语
#define LANGUAGE_CN 1 ///< 简体中文
/** @} */

/** 
 * @defgroup Animation_Timings 动画时长配置
 * @brief 定义了UI动画效果的持续时间 (单位: 毫秒)
 * @{ 
 */
#define ANIM_DURATION_ENTER 800  ///< 页面进入时的初始停留动画时间
#define ANIM_DURATION_ZOOM  600  ///< 页面元素放大/缩小的动画时长
/** @} */

#endif /* __APP_CONFIG_H */
