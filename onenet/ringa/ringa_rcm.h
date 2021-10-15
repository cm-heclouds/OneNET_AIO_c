/**
 * Copyright (c), 2012~2021 iot.10086.cn All Rights Reserved
 *
 * @file ringa_rcm.h
 * @brief Ringa Control Message
 */

#ifndef __RINGA_RCM_H__
#define __RINGA_RCM_H__

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
#define RCM_RESP_CODE_SUCCESS       0
#define RCM_RESP_CODE_NOTSUPPORT    1
#define RCM_RESP_CODE_INVALID       2
#define RCM_RESP_CODE_REPEATE       3
#define RCM_RESP_CODE_UNINITIALIZED 4
#define RCM_RESP_CODE_OFFLINE       5
#define RCM_RESP_CODE_BUSY          6

#define RCM_REQUEST  0
#define RCM_RESPONSE 1
/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/
enum rcm_msg_type_t
{
    RCM_MSG_TYPE_NONE = 0x0,               /** 无效帧*/
    RCM_MSG_TYPE_GET_MODULE_INFO,          /** (MCU->MOD)获取模组信息*/
    RCM_MSG_TYPE_CONFIG_MODULE,            /** (MCU->MOD)模组参数配置*/
    RCM_MSG_TYPE_RESET_MODULE,             /** (MCU->MOD)模组复位*/
    RCM_MSG_TYPE_KEEPALIVE,                /** (MCU->MOD)心跳保活*/
    RCM_MSG_TYPE_START_SMARTCONFIG,        /** (MCU->MOD)设置智能配网*/
    RCM_MSG_TYPE_GET_MODULE_STATUS = 0x08, /** (MCU->MOD)获取模组状态*/
    RCM_MSG_TYPE_GET_NET_TIME,             /** (MCU->MOD)获取网络时间*/
    RCM_MSG_TYPE_NOTIFY_MODULE_STATUS,     /** (MOD->MCU)模组状态变更通知*/
    RCM_MSG_TYPE_TEST,                     /** (MCU->MOD)进入测试模式*/
    RCM_MSG_TYPE_SLEEP,                    /** (MCU->MOD)进入休眠模式*/
    RCM_MSG_TYPE_UPGRADE      = 0x10,      /** (MOD->MCU)固件升级推送*/
    RCM_MSG_TYPE_FUNCS_UPLOAD = 0x13,      /** (MCU->MOD)功能点数据上传*/
    RCM_MSG_TYPE_FUNCS_CMD,                /** (MOD->MCU)功能点命令下发*/
    RCM_MSG_TYPE_FUNCS_QUERY,              /** (MOD->MCU)功能点查询命令*/
    RCM_MSG_TYPE_SET_MAC = 0x1E
};

struct rcm_transceiver_t
{
    int32_t (*send)(uint8_t * /* data*/, uint16_t /* data_len*/);
    int32_t (*recv)(uint8_t * /* data*/, uint16_t /* data_len*/);
};
/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
/**
 * @brief 和物控制消息模块初始化
 *
 * @param send_callback
 * @param parse_buf_len
 * @return
 */
int32_t rcm_init(struct rcm_transceiver_t *transceiver, uint32_t parse_buf_len);

enum rcm_msg_type_t rcm_frame_parse(uint8_t *is_resp, uint8_t **payload, uint16_t *payload_len);
uint32_t rcm_frame_pack_and_send(enum rcm_msg_type_t type, uint8_t is_resp, uint8_t *payload, uint16_t payload_len);

/**
 * @brief 根据
 *
 * @param payload_len
 * @return
 */
uint8_t *rcm_frame_buf_malloc(uint16_t payload_len);
void     rcm_frame_buf_free(uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif
