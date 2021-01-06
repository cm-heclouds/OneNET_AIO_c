/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file at_cmd.c
 * @date 2020/08/03
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "plat_osl.h"
#include "plat_uart.h"
#include "plat_time.h"
#include "err_def.h"
#include "at_ringbuf.h"
#include "at_cmd.h"
#include "log.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define CONFIG_AT_CMD_RINGBUF_LEN 256

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
struct at_port_t
{
    handle_t port;
    at_cmd_recv_cb callback;
    void *ringbuf;
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
static struct at_port_t g_port;

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
static int32_t at_cmd_data_recv(uint8_t *data, uint32_t data_len)
{
    return at_ringbuf_put(g_port.ringbuf, data, data_len);
}

int32_t at_port_open(struct uart_config_t *port_config, at_cmd_recv_cb callback)
{
    g_port.port = uart_open(port_config, at_cmd_data_recv);
    if(-1 == g_port.port)
    {
        return ERR_IO;
    }
    g_port.ringbuf = at_ringbuf_create(CONFIG_AT_CMD_RINGBUF_LEN);
    g_port.callback = callback;

    return ERR_OK;
}

int32_t at_port_close(void)
{
    uart_close(g_port.port);
    at_ringbuf_delete(g_port.ringbuf);
    osl_memset(&g_port, 0, sizeof(g_port));

    return ERR_OK;
}

static int32_t at_parse_line(const int8_t *cmd, const int8_t *expect_prefix, int8_t *data, uint32_t data_len,
                             handle_t cd_hdl)
{
    int32_t ret = ERR_OK;
    int32_t line_len = 0;
    int8_t *line = NULL;
    uint32_t need_next_line = 0;

    do
    {
        line_len = at_ringbuf_parse_line(g_port.ringbuf);
        if(0 < line_len)
        {
            line = osl_malloc(line_len + 1);
            osl_memset(line, 0, line_len + 1);
            at_ringbuf_get(g_port.ringbuf, line, line_len);

            log_info("<==: %s", line);
            
            if(cmd && (0 == osl_strncmp(line, cmd, osl_strlen(cmd) - 2)))
            {
//                log_info("Got Echo: %s", line);
                need_next_line = 1;
            }
            else if(need_next_line)
            {
                if(0 == osl_strncmp(line, expect_prefix, osl_strlen(expect_prefix)))
                {
//                    log_info("Got Expect: %s", line);
                    if(data)
                    {
                        if(line_len > data_len)
                        {
                            return ERR_NO_MEM;
                        }
                        osl_strcpy(data, line + osl_strlen(expect_prefix));
                    }
                    osl_free(line);
                    return ERR_OK;
                }
                else
                {
//                    log_info("Resp Failed: %s", line);
                    return ERR_REQUEST_FAILED;
                }
            }
            else
            {
                if(0 != osl_strncmp(line, "OK", 2))
                {
//                    log_info("Callback: %s", line);
                    g_port.callback(line, line_len);
                    if(data)
                    {
                        if(line_len > data_len)
                        {
                            return ERR_NO_MEM;
                        }
                        osl_strcpy(data, line);
                    }
                    return ERR_OK;
                }
                osl_free(line);
            }
        }
        time_delay_ms(1);
    } while(0 == countdown_is_expired(cd_hdl));

    return ERR_TIMEOUT;
}

int32_t at_step(uint32_t timeout_ms)
{
    int32_t ret = ERR_OK;
    handle_t cd_hdl = countdown_start(timeout_ms);

    ret = at_parse_line(NULL, NULL, NULL, 0, cd_hdl);

    countdown_stop(cd_hdl);
    return ret;
}

int32_t at_send_cmd_with_reply(const int8_t *cmd, const int8_t *expect_code, int8_t *reply_data, uint32_t reply_len,
                               uint32_t timeout_ms)
{
    handle_t cd_hdl = countdown_start(timeout_ms);
    uint32_t cmd_len = osl_strlen(cmd);
    int32_t ret = ERR_REQUEST_FAILED;

    while(ret == ERR_REQUEST_FAILED)
    {
        if(cmd_len != uart_send(g_port.port, (uint8_t *)cmd, cmd_len, countdown_left(cd_hdl)))
        {
            ret = ERR_IO;
            goto exit;
        }

        ret = at_parse_line(cmd, expect_code, reply_data, reply_len, cd_hdl);
        if(ret == ERR_REQUEST_FAILED)
        {
            time_delay_ms(50);
        }
    }

exit:
    countdown_stop(cd_hdl);
    return ret;
}

int32_t at_send_cmd_without_reply(const int8_t *cmd, uint32_t timeout_ms)
{
    handle_t cd_hdl = countdown_start(timeout_ms);
    uint32_t cmd_len = osl_strlen(cmd);
    int32_t ret = ERR_OK;

    if(cmd_len != uart_send(g_port.port, (uint8_t *)cmd, cmd_len, countdown_left(cd_hdl)))
    {
        ret = ERR_IO;
    }

    countdown_stop(cd_hdl);

    return ret;
}
