/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file at_ringbuf.c
 * @date 2020/08/03
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "plat_osl.h"

#include "at_ringbuf.h"
#include "log.h"
/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
struct at_ringbuf_t
{
    uint8_t *buf;
    uint16_t buf_len;
    int16_t rd_offset;
    int16_t wr_offset;
    uint16_t is_full;
};

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
void *at_ringbuf_create(uint32_t buf_size)
{
    struct at_ringbuf_t *ringbuf = NULL;

    if(NULL == (ringbuf = osl_malloc(sizeof(*ringbuf))))
    {
        return ringbuf;
    }
    if(NULL == (ringbuf->buf = osl_malloc(buf_size)))
    {
        osl_free(ringbuf);
        return NULL;
    }
    ringbuf->buf_len = buf_size;
    ringbuf->rd_offset = 0;
    ringbuf->wr_offset = 0;
    ringbuf->is_full = 0;

    return (void *)ringbuf;
}

void at_ringbuf_delete(void *rb)
{
    struct at_ringbuf_t *ringbuf = (struct at_ringbuf_t *)rb;

    if(ringbuf)
    {
        if(ringbuf->buf)
        {
            osl_free(ringbuf->buf);
        }
        osl_free(ringbuf);
    }
    return;
}

int32_t at_ringbuf_parse_line(void *rb)
{
    struct at_ringbuf_t *ringbuf = (struct at_ringbuf_t *)rb;
    uint32_t i = 0;
    uint32_t wr_offset = ringbuf->wr_offset;
    uint32_t rd_offset = ringbuf->rd_offset;
    uint32_t avaliable_bytes =
        (wr_offset >= rd_offset) ? (wr_offset - rd_offset) : (ringbuf->buf_len - rd_offset + wr_offset);
    
    if(avaliable_bytes < 2)
        return 0;

    while((0 < avaliable_bytes) && ('\r' == ringbuf->buf[rd_offset])
          && ('\n' == ringbuf->buf[(rd_offset + 1) % ringbuf->buf_len]))
    {
        avaliable_bytes -= 2;
        rd_offset = (rd_offset + 2) % ringbuf->buf_len;
        ringbuf->rd_offset = rd_offset;
        ringbuf->is_full = 0;
    }

    for(i = 0; i < (avaliable_bytes - 1); i++)
    {
        if(('\r' == ringbuf->buf[(rd_offset + i) % ringbuf->buf_len])
           && ('\n' == ringbuf->buf[(rd_offset + i + 1) % ringbuf->buf_len]))
        {
//            log_info("Found eol offset ==> %d + %d", rd_offset, i+2);
            return (i + 2);
        }
    }
    return 0;
}

int32_t at_ringbuf_get(void *rb, uint8_t *buf, uint32_t buf_len)
{
    struct at_ringbuf_t *ringbuf = (struct at_ringbuf_t *)rb;
    uint32_t rd_offset = ringbuf->rd_offset;
    uint32_t i = 0;

    for(i = 0; i < buf_len; i++)
    {
        buf[i] = ringbuf->buf[(rd_offset + i) % ringbuf->buf_len];
    }
    ringbuf->rd_offset = (rd_offset + buf_len) % ringbuf->buf_len;

    ringbuf->is_full = 0;
    return buf_len;
}

int32_t at_ringbuf_put(void *rb, uint8_t *buf, uint32_t buf_len)
{
    struct at_ringbuf_t *ringbuf = (struct at_ringbuf_t *)rb;
    uint32_t i = 0;
    uint32_t wr_offset = ringbuf->wr_offset;
    uint32_t rd_offset = ringbuf->rd_offset;
    uint32_t free_bytes =
        (wr_offset >= rd_offset) ? (ringbuf->buf_len - wr_offset + rd_offset) : (rd_offset - wr_offset);

    if((ringbuf->is_full) || (free_bytes < buf_len))
        return 0;

    for(i = 0; i < buf_len; i++)
    {
        ringbuf->buf[wr_offset % ringbuf->buf_len] = buf[i];
        wr_offset++;
    }
    ringbuf->wr_offset = wr_offset % ringbuf->buf_len;
    if(ringbuf->wr_offset == ringbuf->rd_offset)
    {
        ringbuf->is_full = 1;
    }

    return buf_len;
}
