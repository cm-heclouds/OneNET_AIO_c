/**
 * Copyright (c), 2012~2021 iot.10086.cn All Rights Reserved
 *
 * @file ringa_api.h
 * @brief
 */

#ifndef __RINGA_API_H__
#define __RINGA_API_H__

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
#if 0
struct ringa_cmd_t
{
    const uint8_t *cmd_id;
    void *cmd_data;
    uint32_t cmd_data_len;
    void *cmd_resp_data;
    uint32_t cmd_resp_data_len;
};

enum ringa_cmd_e
{
    RINGA_CMD_PROPERTY_WRITE = 0,
    RINGA_CMD_PROPERTY_READ,
    RINGA_CMD_UPGRADE
};
#endif

typedef int32_t (*ringa_cmd_cb)(const uint8_t * /* cmd_id*/, void * /* cmd_data*/, uint32_t /* cmd_data_len*/);

enum ringa_ota_event_e
{
    RINGA_OTA_EVENT_UPLOAD_VERSION_SUCCESS = 0,
    RINGA_OTA_EVENT_UPLOAD_VERSION_FAILED,
    RINGA_OTA_EVENT_CHECK_TASK_SUCCESS,
    RINGA_OTA_EVENT_CHECK_TASK_FAILED,
    RINGA_OTA_EVENT_CHECK_TOKEN_SUCCESS,
    RINGA_OTA_EVENT_CHECK_TOKEN_FAILED,
    RINGA_OTA_EVENT_GET_PACKAGE_SUCCESS,
    RINGA_OTA_EVENT_GET_PACKAGE_FAILED,
    RINGA_OTA_EVENT_UPLOAD_STATUS_SUCCESS,
    RINGA_OTA_EVENT_UPLOAD_STATUS_FAILED
};
/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
void    ringa_set_callback(ringa_cmd_cb callback);
int32_t ringa_login(const uint8_t *product_id, const uint8_t *dev_name, const uint8_t *dev_token, uint16_t life_time,
                    uint32_t timeout_ms);
int32_t ringa_logout(uint32_t timeout_ms);
int32_t ringa_notify(uint8_t *raw_data, uint16_t raw_data_len, const uint8_t *ds_id, uint32_t timeout_ms);
int32_t ringa_cmd_resp(const uint8_t *cmd_id, uint8_t *cmd_resp, uint16_t cmd_resp_len, uint32_t timeout_ms);
int32_t ringa_step(uint32_t timeout_ms);

int32_t ringa_notify_sleep(uint32_t timeout_ms);
int32_t ringa_notify_wakeup(uint32_t timeout_ms);

int32_t ringa_ota_start(const uint8_t *product_id, const uint8_t *dev_name, const uint8_t *dev_token,
                        const uint8_t *dev_id, const uint8_t *firmware_version, const uint8_t *software_version);
int32_t ringa_ota_step(uint32_t timeout_ms);
int32_t ringa_ota_stop(void);

#ifdef __cplusplus
}
#endif

#endif
