/**
 * @file app_config.h
 * @brief 配置头文件
 * @details 本文件定义了项目的配置参数，包含应用信息和语言选项。
 * @author SandOcean
 * @date 2025-08-25
 * @version 1.0
 */ 

#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#include "main.h"


/* 应用信息 */
#define APP_NAME "TableClock"
#define APP_AUTHOR "SandOcean."
#define APP_COPYRIGHT "Copyright (c) 2025"
#define APP_VERSION_MAJOR "0"
#define APP_VERSION_MINOR "1"
#define APP_VERSION_PATCH "5"
#define APP_VERSION APP_VERSION_MAJOR "." APP_VERSION_MINOR "." APP_VERSION_PATCH


/* 语言配置 */
#define LANGUAGE_EN 0
#define LANGUAGE_CN 1

#define ANIM_DURATION_ENTER 800  // 初始停留时间
#define ANIM_DURATION_ZOOM  600  // 放大/缩小动画时长


















#endif /* __APP_CONFIG_H */
