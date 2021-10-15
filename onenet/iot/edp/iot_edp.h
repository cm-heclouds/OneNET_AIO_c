/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file iot_edp.h
 * @brief
 */

#ifndef __IOT_EDP_H__
#define __IOT_EDP_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "data_types.h"

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
void *  iot_edp_open(struct iot_cloud_config_t *cloud, iot_dev_event_cb event_cb, uint32_t timeout_ms);
int32_t iot_edp_step(void *dev, uint32_t timeout_ms);
int32_t iot_edp_push_rawdata(void *dev, const uint8_t *target, void *raw_data, uint32_t data_len, uint32_t timeout_ms);
int32_t iot_edp_notify(void *dev, struct iot_data_t *data, uint32_t timeout_ms);
int32_t iot_edp_close(void *dev, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
