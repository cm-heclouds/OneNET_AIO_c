/**
 * @file at_client.h
 * @brief
 */

#ifndef __AT_CLIENT_H__
#define __AT_CLIENT_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "data_types.h"
#include "plat_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/* External Definition ( Constant and Macro )                                */
/*****************************************************************************/

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/
struct at_urc_t
{
    const uint8_t *prefix; // URC数据前缀
    const uint8_t *suffix; // URC数据后缀（不能为NULL或'\0'）
};

typedef void (*at_urc_cb)(uint8_t *data, uint16_t data_len); // URC数据回调

/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
/**
 * @brief 初始化AT模块
 *
 * @param port_config：串口配置信息
 * @param buf_size：AT命令缓冲区大小
 * @return 成功 - AT结构体指针，失败 - NULL
 */
void *at_init(struct uart_config_t *port_config, uint32_t buf_size);

/**
 * @brief 释放AT初始化资源
 *
 * @param ctx：AT结构体指针
 * @return 无
 */
void at_deinit(void *ctx);

/**
 * @brief 清空AT缓冲区数据
 *
 * @param ctx：AT结构体指针
 * @return 无
 */
void at_clear_buf(void *ctx);

/**
 * @brief 设置URC对象
 *
 * @param ctx：AT结构体指针
 * @param urc：URC结构体数组指针
 * @param urc_cnt：URC数据个数
 * @param callback：URC数据回调
 * @return 成功 - ERR_OK，失败 - 相关错误码
 */
int32_t at_set_urc_obj(void *ctx, struct at_urc_t *urc, uint32_t urc_cnt, at_urc_cb callback);

/**
 * @brief 发送AT指令
 *
 * @param ctx：AT结构体指针
 * @param cmd：需要发送的AT字符串
 * @param timeout_ms：超时时间
 * @return >0 - 成功发送的数据长度，其他 - 发送失败
 */
int32_t at_send_cmd(void *ctx, const uint8_t *cmd, uint32_t timeout_ms);

/**
 * @brief 获取并解析指定的AT响应数据（发送AT指令后调用）
 *
 * @param ctx：AT结构体指针
 * @param suffix：指定响应后缀，传入NULL则默认后缀为"\r\n"
 * @param data：数据指针，用于指向AT响应数据
 * @param data_len：响应数据长度
 * @param timeout_ms：超时时间
 * @return 成功 - ERR_OK，失败 - 相关错误码
 */
int32_t at_parse_resp(void *ctx, const uint8_t *suffix, uint8_t **data, uint16_t *data_len, uint32_t timeout_ms);

/**
 * @brief 获取并解析URC数据（循环调用），匹配到指定URC数据后通过回调返回
 *
 * @param ctx：AT结构体指针
 * @param timeout_ms：超时时间
 * @return 成功 - ERR_OK，失败 - 相关错误码
 */
int32_t at_urc_step(void *ctx, uint32_t timeout_ms);

/**
 * @brief 发送指定字节长度AT数据
 *
 * @param ctx：AT结构体指针
 * @param data：需要发送的数据
 * @param data_len：需要发送的数据长度
 * @param timeout_ms：超时时间
 * @return >0 - 成功发送的数据长度，其他 - 发送失败
 */
int32_t at_send_raw_data(void *ctx, uint8_t *data, uint32_t data_len, uint32_t timeout_ms);

/**
 * @brief 获取指定字节长度AT数据
 *
 * @param ctx：AT结构体指针
 * @param data：获取数据的存放地址
 * @param data_len：需要获取的数据长度
 * @param timeout_ms：超时时间
 * @return >0 - 成功获取的数据长度，其他 - 获取失败
 */
int32_t at_recv_raw_data(void *ctx, uint8_t *buf, uint32_t buf_len, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
