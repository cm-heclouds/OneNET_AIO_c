/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file log.c
 * @brief 
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "log.h"
#include "plat_osl.h"

#include <stdarg.h>
#include <stdio.h>

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#if defined(CONFIG_DEBUG_LOG_LEVEL_DEBUG)
#define LOG_LEVEL LOG_LEVEL_DEBUG
#elif defined(CONFIG_DEBUG_LOG_LEVEL_INFO)
#define LOG_LEVEL LOG_LEVEL_INFO
#elif defined(CONFIG_DEBUG_LOG_LEVEL_WARN)
#define LOG_LEVEL LOG_LEVEL_WARN
#elif defined(CONFIG_DEBUG_LOG_LEVEL_ERROR)
#define LOG_LEVEL LOG_LEVEL_ERROR
#endif

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static int8_t log_buf[CONFIG_DEBUG_LOG_BUF_LEN_MAX] = {0};

int8_t *log_level_str[] = {"ERROR", "WARN", "INFO", "DEBUG"};

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
void log_output(const int8_t *function, uint32_t line, uint32_t level, const int8_t *fmt, ...)
{
    va_list ap;
    uint16_t offset = 0;

    if(level > LOG_LEVEL)
    {
        return;
    }
    osl_memset(log_buf, 0, sizeof(log_buf));
    sprintf(log_buf, "[%s]%s %d: ", log_level_str[level], function, line);
    offset = osl_strlen(log_buf);
    va_start(ap, fmt);
    vsnprintf(log_buf + offset, CONFIG_DEBUG_LOG_BUF_LEN_MAX - offset, fmt, ap);
    va_end(ap);
    offset = osl_strlen(log_buf);
    if(log_buf[offset - 1] != '\n')
    {
        log_buf[offset] = '\n';
    }
    printf("%s", log_buf);
}

void log_dump_data(const int8_t *function, uint32_t line, uint8_t *data, uint32_t data_len)
{
    uint32_t i = 0;

    printf("[DUMP]%s %d: ", function, line);
    for(i = 0; i < data_len; i++)
    {
        printf("%02x ", data[i]);
    }
    printf("\n");
}
