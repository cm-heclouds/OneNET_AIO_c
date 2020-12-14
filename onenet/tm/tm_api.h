/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file tm_api.h
 * @brief
 */

#ifndef __TM_API_H__
#define __TM_API_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "data_types.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/* External Definition ( Constant and Macro )                                */
/*****************************************************************************/
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define TM_PROPERTY_RW(x)                                                                                              \
    {                                                                                                                  \
        #x, tm_prop_##x##_rd_cb, tm_prop_##x##_wr_cb                                                                   \
    }

#define TM_PROPERTY_RO(x)                                                                                              \
    {                                                                                                                  \
        #x, tm_prop_##x##_rd_cb, NULL                                                                                  \
    }

#define TM_SERVICE(x)                                                                                                  \
    {                                                                                                                  \
        #x, tm_svc_##x##_cb                                                                                            \
    }

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/
typedef int32_t (*tm_prop_read_cb)(void *res);
typedef int32_t (*tm_prop_write_cb)(void *res);
typedef int32_t (*tm_event_read_cb)(void *res);
typedef int32_t (*tm_svc_invoke_cb)(void *in, void *out);

struct tm_prop_tbl_t
{
    const int8_t *name;
    tm_prop_read_cb tm_prop_rd_cb;
    tm_prop_write_cb tm_prop_wr_cb;
};

struct tm_svc_tbl_t
{
    const int8_t *name;
    tm_svc_invoke_cb tm_svc_cb;
};

struct tm_downlink_tbl_t
{
    struct tm_prop_tbl_t *prop_tbl;
    struct tm_svc_tbl_t *svc_tbl;
    uint16_t prop_tbl_size;
    uint16_t svc_tbl_size;
};
/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
int32_t tm_init(struct tm_downlink_tbl_t *downlink_tbl);
int32_t tm_login(const int8_t *product_id, const int8_t *dev_name, const int8_t *access_key, uint32_t timeout_ms);
int32_t tm_logout(uint32_t timeout_ms);
int32_t tm_post_property(void *prop_data, uint32_t timeout_ms);
int32_t tm_post_event(void *event_data, uint32_t timeout_ms);
int32_t tm_get_desired_props(uint32_t timeout_ms);
int32_t tm_delete_desired_props(uint32_t timeout_ms);

void *tm_pack_device_data(void *data, const int8_t *product_id, const int8_t *dev_name, void *prop, void *event,
                          int8_t as_raw);
int32_t tm_post_pack_data(void *pack_data, uint32_t timeout_ms);
int32_t tm_post_history_data(void *history_data, uint32_t timeout_ms);

#ifdef CONFIG_TM_GATEWAY
typedef int32_t (*tm_subdev_cb)(const int8_t *name, void *data, uint32_t data_len);

int32_t tm_set_subdev_callback(tm_subdev_cb callback);

int32_t tm_post_raw(const int8_t *name, uint8_t *raw_data, uint32_t raw_data_len, uint8_t **reply_data,
                    uint32_t *reply_data_len, uint32_t timeout_ms);
int32_t tm_send_request(const int8_t *name, uint8_t as_raw, void *data, uint32_t data_len, void **reply_data,
                        uint32_t *reply_data_len, uint32_t timeout_ms);
int32_t tm_send_response(const int8_t *name, int8_t *msg_id, int32_t msg_code, uint8_t as_raw, void *resp_data,
                         uint32_t resp_data_len, uint32_t timeout_ms);
#endif
int32_t tm_step(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
