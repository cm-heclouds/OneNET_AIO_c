/**
 * Copyright (c), 2012~2021 iot.10086.cn All Rights Reserved
 *
 * @file ringa_cmd.h
 * @brief
 */

#ifndef __RINGA_RINGA_CMD_H__
#define __RINGA_RINGA_CMD_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "data_types.h"

#include "ringa_tlv.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/* External Definition ( Constant and Macro )                                */
/*****************************************************************************/
#define RINGA_CMD_TYPE_SYSTEM    0x0001
#define RINGA_CMD_SYSTEM_BIND    ((RINGA_CMD_TYPE_SYSTEM << 16) | 0x0001)
#define RINGA_CMD_SYSTEM_UNBIND  ((RINGA_CMD_TYPE_SYSTEM << 16) | 0x0002)
#define RINGA_CMD_SYSTEM_UPGRADE ((RINGA_CMD_TYPE_SYSTEM << 16) | 0x0003)

#define RINGA_CMD_TYPE_DEVICE          0x0002
#define RINGA_CMD_DEVICE_META          ((RINGA_CMD_TYPE_DEVICE << 16) | 0x0001)
#define RINGA_CMD_DEVICE_ONLINE        ((RINGA_CMD_TYPE_DEVICE << 16) | 0x0002)
#define RINGA_CMD_DEVICE_OFFLINE       ((RINGA_CMD_TYPE_DEVICE << 16) | 0x0003)
#define RINGA_CMD_DEVICE_TOKEN_REQ     ((RINGA_CMD_TYPE_DEVICE << 16) | 0x0004)
#define RINGA_CMD_DEVICE_TOKEN_RESP    ((RINGA_CMD_TYPE_DEVICE << 16) | 0x0005)
#define RINGA_CMD_DEVICE_SLEEP_STATUS  ((RINGA_CMD_TYPE_DEVICE << 16) | 0x0006)
#define RINGA_CMD_DEVICE_WAKEUP_STATUS ((RINGA_CMD_TYPE_DEVICE << 16) | 0x0007)

#define RINGA_CMD_TYPE_PROPERTY_NOTIFY 0x0003

#define RINGA_CMD_TYPE_PROPERTY_WIRTE 0x0004

#define RINGA_CMD_TYPE_PROPERTY_READ 0x0005

#define RINGA_CMD_TYPE_EVENT_NOTIFY 0x0006

#define RINGA_CMD_TYPE_GET(cmd) ((cmd >> 16) & 0xFFFF)

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/

/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
int32_t ringa_cmd_pack(uint8_t *buf, uint32_t cmd, enum ringa_tlv_type_e type, uint8_t *cmd_data,
                       uint16_t cmd_data_len);
int32_t ringa_cmd_get_data_len(uint8_t *buf, uint32_t *cmd, uint16_t *cmd_data_len);
int32_t ringa_cmd_parse(uint8_t *buf, uint32_t *cmd, enum ringa_tlv_type_e *type, uint8_t **cmd_data,
                        uint16_t *cmd_data_len);
#ifdef __cplusplus
}
#endif

#endif
