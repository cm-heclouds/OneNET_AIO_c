/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 * 
 * @file at_cmd.h
 * @date 2020/08/03
 * @brief 
 */

#ifndef __AT_CMD_H__
#define __AT_CMD_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "data_types.h"
#include "plat_uart.h"

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
typedef int32_t (*at_cmd_recv_cb)(const int8_t *cmd, uint32_t cmd_len);

/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
int32_t at_port_open(struct uart_config_t *port_config, at_cmd_recv_cb callback);
int32_t at_port_close(void);
int32_t at_step(uint32_t timeout_ms);
int32_t at_send_cmd_with_reply(const int8_t *cmd, const int8_t *expect_prefix, int8_t *reply_data, uint32_t reply_len, uint32_t timeout_ms);
int32_t at_send_cmd_without_reply(const int8_t *cmd, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
