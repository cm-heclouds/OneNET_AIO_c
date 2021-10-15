/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file tm_subdev.h
 * @brief
 */

#ifndef __TM_SUBDEV_H__
#define __TM_SUBDEV_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "data_types.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifdef CONFIG_TM_GATEWAY

/*****************************************************************************/
/* External Definition ( Constant and Macro )                                */
/*****************************************************************************/

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/
struct tm_subdev_cbs
{
    int32_t (*subdev_props_get)(const uint8_t *product_id, const uint8_t *dev_name, const uint8_t *props_list,
                                uint8_t **props_data);
    int32_t (*subdev_props_set)(const uint8_t *product_id, const uint8_t *dev_name, uint8_t *props_data);
    int32_t (*subdev_service_invoke)(const uint8_t *product_id, const uint8_t *dev_name, const uint8_t *svc_id,
                                     uint8_t *in_data, uint8_t **out_data);
    int32_t (*subdev_topo)(uint8_t *topo_data);
};

/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
int32_t tm_subdev_init(struct tm_subdev_cbs callbacks);
int32_t tm_subdev_add(const uint8_t *product_id, const uint8_t *dev_name, const uint8_t *access_key,
                      uint32_t timeout_ms);
int32_t tm_subdev_delete(const uint8_t *product_id, const uint8_t *dev_name, const uint8_t *access_key,
                         uint32_t timeout_ms);
int32_t tm_subdev_topo_get(uint32_t timeout_ms);
int32_t tm_subdev_login(const uint8_t *product_id, const uint8_t *dev_name, uint32_t timeout_ms);
int32_t tm_subdev_logout(const uint8_t *product_id, const uint8_t *dev_name, uint32_t timeout_ms);

int32_t tm_subdev_post_data(const uint8_t *product_id, const uint8_t *dev_name, uint8_t *prop_json, uint8_t *event_json,
                            uint32_t timeout_ms);

#endif
#ifdef __cplusplus
}
#endif

#endif
