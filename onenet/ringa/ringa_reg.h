/**
 * Copyright (c), 2012~2021 iot.10086.cn All Rights Reserved
 *
 * @file ringa_reg.h
 * @brief
 */

#ifndef __RINGA_REG_H__
#define __RINGA_REG_H__

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
enum ringa_reg_event_e
{
    RINGA_REG_EVT_CONNECTED,
    RINGA_REG_EVT_DISCONNECTED,
    RINGA_REG_EVT_SUCCESSED,
    RINGA_REG_EVT_FAILED,
    RINGA_REG_EVT_TIMEOUT
};

struct ringa_reg_result_t
{
    uint8_t *dev_token;
    uint8_t *dev_id;
};

typedef int32_t (*ringa_reg_evt_cb)(enum ringa_reg_event_e /* evt*/, void * /* evt_data*/, uint32_t /* evt_data_len*/);

/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
int32_t ringa_register(const uint8_t *product_id, const uint8_t *dev_name, const uint8_t *connection_id,
                       const uint8_t *binding_token, ringa_reg_evt_cb reg_callback, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
