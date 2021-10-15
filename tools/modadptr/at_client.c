/**
 * @file at_client.c
 * @brief AT接口封装
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "at_client.h"
#include "plat_uart.h"
#include "plat_time.h"
#include "plat_osl.h"
#include "err_def.h"
#include "ringbuf.h"
#include "log.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define AT_END_CR_LF "\r\n"

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
struct at_ctx_t
{
    struct at_urc_t *urc;

    uint32_t  urc_cnt;
    at_urc_cb urc_cb;

    handle_t handle;
    uint8_t *buf;
    uint32_t buf_len;
    int32_t  wr_oft;
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
void *at_init(struct uart_config_t *port_config, uint32_t buf_size)
{
    struct at_ctx_t *at_ctx = NULL;

    if (NULL == (at_ctx = osl_calloc(1, sizeof(*at_ctx))))
    {
        return NULL;
    }
    if (NULL == (at_ctx->buf = osl_calloc(1, buf_size)))
    {
        goto exit;
    }
    if (-1 == (at_ctx->handle = uart_open(port_config)))
    {
        goto exit1;
    }
    at_ctx->buf_len = buf_size;
    at_ctx->wr_oft  = 0;

    return (void *)at_ctx;

exit1:
    osl_free(at_ctx->buf);
exit:
    osl_free(at_ctx);
    return NULL;
}

void at_deinit(void *ctx)
{
    struct at_ctx_t *at_ctx = (struct at_ctx_t *)ctx;

    if (at_ctx)
    {
        if (at_ctx->buf)
        {
            osl_free(at_ctx->buf);
        }
        osl_free(at_ctx);
    }
}

void at_clear_buf(void *ctx)
{
    struct at_ctx_t *at_ctx = (struct at_ctx_t *)ctx;

    if (NULL != at_ctx->buf)
    {
        osl_memset(at_ctx->buf, 0, at_ctx->buf_len);
        at_ctx->wr_oft = 0;
    }
}

int32_t at_set_urc_obj(void *ctx, struct at_urc_t *urc, uint32_t urc_cnt, at_urc_cb callback)
{
    struct at_ctx_t *at_ctx = (struct at_ctx_t *)ctx;

    uint32_t i = 0;

    for (i = 0; i < urc_cnt; i++)
    {
        if (NULL == urc[i].suffix)
        {
            return ERR_INVALID_DATA;
        }
    }
    at_ctx->urc     = urc;
    at_ctx->urc_cnt = urc_cnt;
    at_ctx->urc_cb  = callback;

    return ERR_OK;
}

static int32_t at_get_urc_obj(void *ctx)
{
    struct at_ctx_t *at_ctx = (struct at_ctx_t *)ctx;

    uint8_t *buf     = at_ctx->buf;
    int32_t  buf_len = at_ctx->wr_oft;
    uint16_t pfx_len = 0;
    uint16_t sfx_len = 0;
    uint16_t cnt     = 0;
    int32_t  i, j, k = 0;

    if (0 == buf_len)
    {
        return ERR_INVALID_DATA;
    }

    for (i = 0; i < at_ctx->urc_cnt; i++)
    {
        pfx_len = (NULL != at_ctx->urc[i].prefix) ? osl_strlen(at_ctx->urc[i].prefix) : 0;
        sfx_len = osl_strlen(at_ctx->urc[i].suffix);
        if (buf_len < pfx_len + sfx_len)
        {
            continue;
        }

        if (0 == osl_strncmp(buf + buf_len - sfx_len, at_ctx->urc[i].suffix, sfx_len)) //找到后缀
        {
            if (0 != pfx_len)
            {
                for (j = 0; j <= buf_len - pfx_len - sfx_len; j++)
                {
                    for (k = 0; k < pfx_len; k++)
                    {
                        if (at_ctx->urc[i].prefix[k] == buf[j + k])
                        {
                            if (++cnt == pfx_len) //找到前缀
                            {
                                at_ctx->urc_cb(buf + j, buf_len - j);
                                return ERR_OK;
                            }
                        }
                        else
                        {
                            cnt = 0;
                            break;
                        }
                    }
                }
            }
            else
            {
                at_ctx->urc_cb(buf, buf_len);
                return ERR_OK;
            }
        }
    }

    return ERR_INVALID_DATA;
}

int32_t at_send_cmd(void *ctx, const uint8_t *cmd, uint32_t timeout_ms)
{
    struct at_ctx_t *at_ctx = (struct at_ctx_t *)ctx;

    at_urc_step(at_ctx, 50); //检测接收缓冲区是否存在URC数据

    log_debug("at send: %s\n", cmd);
    return uart_send(at_ctx->handle, (uint8_t *)cmd, osl_strlen(cmd), timeout_ms);
}

static int32_t at_parse_line(void *ctx, const uint8_t *suffix, uint8_t **data, uint16_t *data_len, uint8_t is_resp,
                             uint32_t timeout_ms)
{
    struct at_ctx_t *at_ctx  = (struct at_ctx_t *)ctx;
    const uint8_t *  sfx_ptr = (NULL != suffix) ? suffix : AT_END_CR_LF;
    uint16_t         sfx_len = osl_strlen(sfx_ptr);

    handle_t cd_hdl = countdown_start(timeout_ms);
    int32_t  recv   = 0;
    uint8_t  ch     = 0;
    int32_t  ret    = ERR_TIMEOUT;

    if (1 == is_resp)
    {
        at_clear_buf(at_ctx); //清空AT缓冲区多余数据
    }
    do
    {
        if (at_ctx->wr_oft < at_ctx->buf_len - 1) //最后一位为'\0'
        {
            if (0 < (recv = uart_recv(at_ctx->handle, &ch, 1, 10))) //每次从串口接收缓冲区取1个字节
            {
                at_ctx->buf[at_ctx->wr_oft++] = ch;
            }
        }
        else
        {
            log_debug("at parse fail, data length is out of buffer size\n");
            ret = ERR_OVERFLOW;
            goto exit1;
        }

        /** 检测URC数据 */
        if (ERR_OK == at_get_urc_obj(at_ctx))
        {
            // log_debug("find urc obj, at_buf_wr_oft = %d\n", at_ctx->wr_oft);
            // log_dump(at_ctx->buf, at_ctx->wr_oft);
            at_clear_buf(at_ctx);
            if (0 == is_resp)
            {
                ret = ERR_OK;
                goto exit;
            }
        }

        /** 检测AT响应 */
        if (1 == is_resp && sfx_len <= at_ctx->wr_oft)
        {
            if (0 == osl_strncmp(at_ctx->buf + at_ctx->wr_oft - sfx_len, sfx_ptr, sfx_len))
            {
                log_debug("at got suffix: %s\n", at_ctx->buf);
                if (NULL != data)
                {
                    *data     = at_ctx->buf;
                    *data_len = at_ctx->wr_oft;
                }
                ret = ERR_OK;
                goto exit;
            }
        }
    } while ((0 < recv) || (0 == countdown_is_expired(cd_hdl))); //接收缓冲区有数据，超时也继续读取
    // log_debug("at parse timeout, at_buf_wr_oft = %d\n", at_ctx->wr_oft);
    // log_dump(at_ctx->buf, at_ctx->wr_oft);
    goto exit;

exit1:
    // log_debug("at parse exit, at_buf_wr_oft = %d\n", at_ctx->wr_oft);
    // log_dump(at_ctx->buf, at_ctx->wr_oft);
    at_clear_buf(at_ctx);
exit:
    countdown_stop(cd_hdl);
    return ret;
}

int32_t at_parse_resp(void *ctx, const uint8_t *suffix, uint8_t **data, uint16_t *data_len, uint32_t timeout_ms)
{
    return at_parse_line(ctx, suffix, data, data_len, 1, timeout_ms);
}

int32_t at_urc_step(void *ctx, uint32_t timeout_ms)
{
    return at_parse_line(ctx, NULL, NULL, NULL, 0, timeout_ms);
}

int32_t at_send_raw_data(void *ctx, uint8_t *data, uint32_t data_len, uint32_t timeout_ms)
{
    struct at_ctx_t *at_ctx = (struct at_ctx_t *)ctx;

    at_urc_step(at_ctx, 50);

    return uart_send(at_ctx->handle, data, data_len, timeout_ms);
}

int32_t at_recv_raw_data(void *ctx, uint8_t *buf, uint32_t buf_len, uint32_t timeout_ms)
{
    struct at_ctx_t *at_ctx = (struct at_ctx_t *)ctx;

    return uart_recv(at_ctx->handle, buf, buf_len, timeout_ms);
}
