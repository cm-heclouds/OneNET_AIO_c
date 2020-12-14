/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 * 
 * @file at_ringbuf.h
 * @date 2020/08/03
 * @brief 
 */

#ifndef __AT_RINGBUF_H__
#define __AT_RINGBUF_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "data_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*****************************************************************************/
/* External Definition ( Constant and Macro )                                */
/*****************************************************************************/

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/

/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
void* at_ringbuf_create(uint32_t buf_size);
void at_ringbuf_delete(void *rb);
int32_t at_ringbuf_parse_line(void *rb);
int32_t at_ringbuf_get(void *rb, uint8_t *buf, uint32_t buf_len);
int32_t at_ringbuf_put(void *rb, uint8_t *buf, uint32_t buf_len);

#ifdef __cplusplus
}
#endif

#endif
