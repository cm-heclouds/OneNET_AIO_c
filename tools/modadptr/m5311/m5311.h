/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file m5311.h
 * @date 2020/07/31
 * @brief
 */

#ifndef __M5311_H__
#define __M5311_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "data_types.h"
#include "nbiot_m2m.h"

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
int32_t m5311_init(void *arg, m2m_event_cb callback, uint32_t timeout_ms);
void    m5311_deinit(void);
int32_t m5311_mipl_create(const uint8_t *host, uint32_t life_time, const uint8_t *authcode, const uint8_t *psk,
                          uint8_t flag, uint32_t timeout_ms);
int32_t m5311_mipl_delete(uint32_t timeout_ms);
int32_t m5311_mipl_open(uint32_t life_time, uint32_t timeout_ms);
int32_t m5311_mipl_close(uint32_t timeout_ms);
int32_t m5311_mipl_addobj(uint32_t obj_id, uint32_t inst_map, uint32_t timeout_ms);
int32_t m5311_mipl_delobj(uint32_t obj_id, uint32_t timeout_ms);
int32_t m5311_mipl_update(uint32_t life_time, boolean update_objs, uint32_t timeout_ms);
int32_t m5311_mipl_step(uint32_t timeout_ms);
int32_t m5311_mipl_notify_with_reply(uint32_t msg_id, struct m2m_data_t *data, uint32_t timeout_ms);
int32_t m5311_mipl_notify_without_reply(uint32_t msg_id, struct m2m_data_t *data, uint32_t timeout_ms);
int32_t m5311_mipl_discovery_resp(uint32_t obj_id, const uint8_t *res_list, uint32_t timeout_ms);

#if 1
int32_t m5311_mipl_read_resp(uint32_t msg_id, uint32_t result_code, struct m2m_data_t *data, uint32_t timeout_ms);
int32_t m5311_mipl_write_resp(uint32_t msg_id, uint32_t result_code, uint32_t timeout_ms);
int32_t m5311_mipl_execute_resp(uint32_t msg_id, uint32_t result_code, uint32_t timeout_ms);
#endif

#ifdef __cplusplus
}
#endif

#endif
