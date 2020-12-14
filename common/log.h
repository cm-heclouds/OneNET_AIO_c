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
#define log_debug(fmt, ...) log_output(__FUNCTION__, __LINE__, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__);
#define log_info(fmt, ...)  log_output(__FUNCTION__, __LINE__, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__);
#define log_error(fmt, ...) log_output(__FUNCTION__, __LINE__, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__);
#define log_warn(fmt, ...) log_output(__FUNCTION__, __LINE__, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__);
#define log_dump(data, data_len) log_dump_data(__FUNCTION__, __LINE__, data, data_len);
#else
#define log_debug(fmt, ...)
#define log_info(fmt, ...)
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
void log_output(const int8_t *function, uint32_t line, uint32_t level, const int8_t *fmt, ...);
void log_dump_data(const int8_t *function, uint32_t line, uint8_t *data, uint32_t data_len);
#endif

#ifdef __cplusplus
}
#endif

#endif
