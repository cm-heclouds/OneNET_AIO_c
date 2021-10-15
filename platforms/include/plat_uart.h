/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file plat_uart.h
 * @brief
 */

#ifndef __PLAT_UART_H__
#define __PLAT_UART_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "data_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/* External Definition ( Constant and Macro )                                */
/*****************************************************************************/

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/
enum uart_data_bits_e
{
    UART_DATABITS_5 = 0,
    UART_DATABITS_6,
    UART_DATABITS_7,
    UART_DATABITS_8
};

enum uart_stop_bits_e
{
    UART_STOPBITS_1 = 0,
    UART_STOPBITS_1_5,
    UART_STOPBITS_2
};

enum uart_flow_ctrl_e
{
    UART_FLOWCTRL_NONE = 0,
    UART_FLOWCTRL_RTS,
    UART_FLOWCTRL_CTS,
    UART_FLOWCTRL_CTS_RTS
};

enum uart_parity_e
{
    UART_PARITY_NONE = 0,
    UART_PARITY_ODD,
    UART_PARITY_EVEN
};

struct uart_config_t
{
    uint32_t              baudrate;
    enum uart_data_bits_e data_bits;
    enum uart_stop_bits_e stop_bits;
    enum uart_flow_ctrl_e flow_ctrl;
    enum uart_parity_e    parity;
};

/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
/**
 * @brief 打开用于模组通信的串口。串口端口号由接口内部指定，上层并不关心，也无法获知。
 *
 * @param port_config 串口配置参数
 * @return handle_t 通信串口操作句柄，失败返回-1
 */
handle_t uart_open(struct uart_config_t *port_config);

/**
 * @brief 通过通信串口发送数据
 *
 * @param uart 通信串口操作句柄
 * @param data 需要发送的数据缓冲区地址
 * @param data_len 需要发送的数据长度
 * @param timeout_ms 数据发送超时时间
 * @retval   <0 - 发送失败
 * @retval 其它 - 实际发送的数据长度
 */
int32_t uart_send(handle_t uart, uint8_t *data, uint32_t data_len, uint32_t timeout_ms);

/**
 * @brief 通过通信串口接收数据
 *
 * @param uart 通信串口操作句柄
 * @param buf 用于接收数据的缓冲区地址
 * @param buf_len 需要接收的数据长度
 * @param timeout_ms 接收数据的超时时间
 * @retval   <0 - 接收失败
 * @retval 其它 - 实际接收的数据长度
 */
int32_t uart_recv(handle_t uart, uint8_t *buf, uint32_t buf_len, uint32_t timeout_ms);

/**
 * @brief 关闭通信串口
 *
 * @param uart 通信串口操作句柄
 */
void uart_close(handle_t uart);

#ifdef __cplusplus
}
#endif

#endif
