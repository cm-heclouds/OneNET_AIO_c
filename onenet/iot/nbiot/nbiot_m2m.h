/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file nbiot_m2m.h
 * @brief
 */

#ifndef __NBIOT_M2M_H__
#define __NBIOT_M2M_H__

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
enum m2m_event_e
{
    /** 无参数*/
    M2M_EVENT_LOGIN_OK,
    M2M_EVENT_LOGIN_FAILED,
    M2M_EVENT_UPDATE_OK,

    M2M_EVENT_RES_NOTIFY_OK,

    M2M_EVENT_RES_READ,
    M2M_EVENT_RES_WRITE,
    M2M_EVENT_RES_EXECUTE,

    M2M_EVENT_LOGOUT_OK
};

enum m2m_data_type_e
{
    M2M_DT_STRING = 1,
    M2M_DT_OPAQUE,
    M2M_DT_INTEGER,
    M2M_DT_FLOAT,
    M2M_DT_BOOL,
    M2M_DT_HEX_STR
};

enum m2m_attribute_e
{
    M2M_ATTR_READ = 1,
    M2M_ATTR_WRITE,
    M2M_ATTR_EXEC,
};

struct m2m_res_info_t
{
    int32_t obj_id;
    int32_t inst_id;
    int32_t res_id;
};

struct m2m_data_t
{
    struct m2m_res_info_t res_info;
    uint32_t              data_type;
    union
    {
        uint8_t *val_str;
        struct
        {
            uint8_t *data;
            uint32_t data_len;
        } val_opa;
        int64_t   val_int;
        float64_t val_float;
        boolean   val_bool;
    } data;
};

struct m2m_attr_t
{
    uint32_t             msg_id;
    enum m2m_attribute_e attr_type;
    struct m2m_data_t    data_info;

    uint32_t attr_result;
};

typedef int32_t (*m2m_event_cb)(void *, enum m2m_event_e, uint8_t *, uint32_t);
/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
void *m2m_init(void *arg, struct network_param_t *net, m2m_event_cb callback, uint32_t timeout_ms);

int32_t m2m_deinit(void *arg, uint32_t timeout_ms);

int32_t m2m_register(void *arg, uint8_t *auth_code, uint32_t life_time, uint32_t timeout_ms);

int32_t m2m_update(uint32_t life_time, uint32_t timeout_ms);

int32_t m2m_deregister(void *arg, uint32_t timeout_ms);

int32_t m2m_add_res(void *arg, struct m2m_res_info_t *res_info);

int32_t m2m_del_res(void *arg, struct m2m_res_info_t *res_info);

int32_t m2m_notify_with_reply(uint32_t msg_id, struct m2m_data_t *data, uint8_t *reply, uint32_t *reply_len,
                              uint32_t timeout_ms);

int32_t m2m_notify_without_reply(uint32_t msg_id, struct m2m_data_t *data, uint32_t timeout_ms);

int32_t m2m_read_reply(uint32_t msg_id, uint32_t result_code, struct m2m_data_t *data, uint32_t timeout_ms);
int32_t m2m_write_reply(uint32_t msg_id, uint32_t result_code, uint32_t timeout_ms);

int32_t m2m_step(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
