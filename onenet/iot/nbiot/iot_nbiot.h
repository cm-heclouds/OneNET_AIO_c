/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file iot_nbiot.h
 * @date 2020/01/09
 * @brief
 */

#ifndef __IOT_NBIOT_H__
#define __IOT_NBIOT_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include <iot_api.h>

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
void *  iot_nbiot_open(struct iot_cloud_config_t *cloud, iot_dev_event_cb event_cb, uint32_t timeout_ms);
int32_t iot_nbiot_step(void *dev, uint32_t timeout_ms);
int32_t iot_nbiot_push_rawdata(void *dev, const uint8_t *target, void *raw_data, uint32_t data_len,
                               uint32_t timeout_ms);
int32_t iot_nbiot_notify(void *dev, struct iot_data_t *data, uint32_t timeout_ms);
int32_t iot_nbiot_observe(void *dev, const uint8_t *resource, uint32_t timeout_ms);
int32_t iot_nbiot_observe_cancel(void *dev, const uint8_t *resource, uint32_t timeout_ms);
int32_t iot_nbiot_close(void *dev, uint32_t timeout_ms);
#ifdef __cplusplus
}
#endif

#endif
