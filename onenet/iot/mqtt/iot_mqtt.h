/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file iot_mqtt.h
 * @date 2020/07/16
 * @brief
 */

#ifndef __IOT_MQTT_H__
#define __IOT_MQTT_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "iot_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/* External Definition ( Constant and Macro )                                */
/*****************************************************************************/

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/

/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
void *  iot_mqtt_open(struct iot_cloud_config_t *cloud, iot_dev_event_cb event_cb, uint32_t timeout_ms);
int32_t iot_mqtt_step(void *dev, uint32_t timeout_ms);
int32_t iot_mqtt_push_rawdata(void *dev, const uint8_t *target, void *raw_data, uint32_t data_len, uint32_t timeout_ms);
int32_t iot_mqtt_notify(void *dev, struct iot_data_t *data, uint32_t timeout_ms);
int32_t iot_mqtt_observe(void *dev, const uint8_t *resource, uint32_t timeout_ms);
int32_t iot_mqtt_observe_cancel(void *dev, const uint8_t *resource, uint32_t timeout_ms);
int32_t iot_mqtt_close(void *dev, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
