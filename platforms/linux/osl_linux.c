/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file osl_linux.c
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "plat_osl.h"
#include "plat_time.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
void *osl_malloc(size_t size)
{
    return malloc(size);
}

void *osl_calloc(size_t num, size_t size)
{
    return calloc(num, size);
}

void osl_free(void *ptr)
{
    free(ptr);
}

void *osl_memcpy(void *dst, const void *src, size_t n)
{
    return memcpy(dst, src, n);
}

void *osl_memmove(void *dst, const void *src, size_t n)
{
    return memmove(dst, src, n);
}

void *osl_memset(void *dst, int32_t val, size_t n)
{
    return memset(dst, val, n);
}

uint8_t *osl_strdup(const uint8_t *s)
{
    return (uint8_t *)strdup((const char *)s);
}

uint8_t *osl_strndup(const uint8_t *s, size_t n)
{
    return (uint8_t *)strndup((const char *)s, n);
}

uint8_t *osl_strcpy(uint8_t *s1, const uint8_t *s2)
{
    return (uint8_t *)strcpy((char *)s1, (const char *)s2);
}

uint32_t osl_strlen(const uint8_t *s)
{
    return strlen((const char *)s);
}

uint8_t *osl_strcat(uint8_t *dst, const uint8_t *src)
{
    return (uint8_t *)strcat((char *)dst, (const char *)src);
}

int32_t osl_strcmp(const uint8_t *s1, const uint8_t *s2)
{
    return strcmp((const char *)s1, (const char *)s2);
}

int32_t osl_strncmp(const uint8_t *s1, const uint8_t *s2, size_t n)
{
    return strncmp((const char *)s1, (const char *)s2, n);
}

uint8_t *osl_strstr(const uint8_t *s1, const uint8_t *s2)
{
    return (uint8_t *)strstr((const char *)s1, (const char *)s2);
}

int32_t osl_sprintf(uint8_t *str, const uint8_t *format, ...)
{
    va_list ap;
    int     ret = 0;

    va_start(ap, format);
    ret = vsprintf((char *)str, (char *)format, ap);
    va_end(ap);

    return ret;
}

int32_t osl_sscanf(const uint8_t *str, const uint8_t *format, ...)
{
    va_list ap;
    int     ret = 0;

    va_start(ap, format);
    ret = vsscanf((const char *)str, (const char *)format, ap);
    va_end(ap);

    return ret;
}

void osl_assert(boolean expression)
{
    assert(expression);
}

int32_t osl_get_random(unsigned char *buf, size_t len)
{
    size_t i = 0;

    srand(time_count());
    for (i = 0; i < len; i++)
    {
        buf[i] = (rand() & 0xFF);
    }
    return 0;
}

int32_t osl_atoi(const uint8_t *nptr)
{
    return atoi((const char *)nptr);
}
