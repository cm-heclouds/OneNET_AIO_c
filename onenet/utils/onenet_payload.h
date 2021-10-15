/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file onenet_payload.h
 * @brief
 */

#ifndef __ONENET_PAYLOAD_H__
#define __ONENET_PAYLOAD_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "data_types.h"
#include "iot_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/* External Definition（Constant and Macro )                                 */
/*****************************************************************************/

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/

/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
uint32_t iot_data_to_payload(uint8_t *payload, struct iot_data_t *data);

#ifdef __cplusplus
}
#endif

#endif
