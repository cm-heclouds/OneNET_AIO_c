/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file iot_mqtts.h
 * @date 2020/01/07
 * @brief
 */

#ifndef __IOT_MQTTS_H__
#define __IOT_MQTTS_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "iot_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/* External Definition ( Constant and Macro )                                 */
/*****************************************************************************/

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/

/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
void *iot_mqtts_open(struct iot_cloud_config_t *cloud, iot_dev_event_cb event_cb, uint32_t timeout_ms);
#if 0
int32_t iot_mqtts_get_info(void *dev, struct iot_dev_info_t *dev_info);
#endif
int32_t iot_mqtts_step(void *dev, uint32_t timeout_ms);
int32_t iot_mqtts_push_rawdata(void *dev, const uint8_t *target, void *raw_data, uint32_t data_len,
                               uint32_t timeout_ms);
int32_t iot_mqtts_notify(void *dev, struct iot_data_t *data, uint32_t timeout_ms);
int32_t iot_mqtts_close(void *dev, uint32_t timeout_ms);
int32_t iot_mqtts_observe(void *dev, const uint8_t *resource, uint32_t timeout_ms);
int32_t iot_mqtts_observe_cancel(void *dev, const uint8_t *resource, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
