/**
 * Copyright (c), 2012~2018 iot.10086.cn All Rights Reserved
 * @file        log.h
 * @brief       统一日志输出接口。
 */

#ifndef __LOG_H__
#define __LOG_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "data_types.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/* External Definition Constant and Macro )                                 */
/*****************************************************************************/
/**
 * @brief 日志等级 - 异常
 *
 */
#define LOG_LEVEL_ERROR 0

/**
 * @brief 日志等级 - 警告
 *
 */
#define LOG_LEVEL_WARN 1

/**
 * @brief 日志等级 - 消息
 *
 */
#define LOG_LEVEL_INFO 2

/**
 * @brief 日志等级 - 调试信息
 *
 */
#define LOG_LEVEL_DEBUG 3

#ifdef CONFIG_DEBUG_LOG
#if defined(CONFIG_DEBUG_LOG_LEVEL_DEBUG)
#define LOG_LEVEL LOG_LEVEL_DEBUG
#elif defined(CONIG_DEBUG_LOG_LEVEL_INFO)
#define LOG_LEVEL LOG_LEVEL_INFO
#elif defined(CONFIG_DEBUG_LOG_LEVEL_WARN)
#define LOG_LEVEL LOG_LEVEL_WARN
#elif defined(CONFIG_DEBUG_LOG_LEVEL_ERROR)
#define LOG_LEVEL LOG_LEVEL_ERROR
#endif

#if (LOG_LEVEL_DEBUG <= LOG_LEVEL)
#define log_debug(fmt, ...) printf("[DEBUG]%s %d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define log_debug(fmt, ...)
#endif

#if (LOG_LEVEL_INFO <= LOG_LEVEL)
#define log_info(fmt, ...) printf("[INFO]%s %d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define log_info(fmt, ...)
#endif

#if (LOG_LEVEL_WARN <= LOG_LEVEL)
#define log_warn(fmt, ...) printf("[WARN]%s %d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define log_warn(fmt, ...)
#endif

#if (LOG_LEVEL_ERROR <= LOG_LEVEL)
#define log_error(fmt, ...) printf("[ERROR]%s %d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define log_error(fmt, ...)
#endif

#define log_dump(data, data_len) log_dump_data(__FUNCTION__, __LINE__, data, data_len)
#else
#define log_debug(fmt, ...)
#define log_info(fmt, ...)
#define log_warn(fmt, ...)
#define log_error(fmt, ...)
#define log_dump(data, data_len)
#endif

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/

/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
#ifdef CONFIG_DEBUG_LOG
void log_dump_data(const uint8_t *function, uint32_t line, uint8_t *data, uint32_t data_len);
#endif

#ifdef __cplusplus
}
#endif

#endif
