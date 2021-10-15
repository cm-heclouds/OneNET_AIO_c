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

#ifdef CONFIG_GENERAL_OTA
typedef int32_t (*tm_ota_cb)(void);
#endif

struct tm_prop_tbl_t
{
    const uint8_t *  name;
    tm_prop_read_cb  tm_prop_rd_cb;
    tm_prop_write_cb tm_prop_wr_cb;
};

struct tm_svc_tbl_t
{
    const uint8_t *  name;
    tm_svc_invoke_cb tm_svc_cb;
};

struct tm_downlink_tbl_t
{
    struct tm_prop_tbl_t *prop_tbl;
    struct tm_svc_tbl_t * svc_tbl;
    uint16_t              prop_tbl_size;
    uint16_t              svc_tbl_size;
#ifdef CONFIG_GENERAL_OTA
    tm_ota_cb ota_cb;
#endif
};
/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
/**
 * @brief 设备初始化
 *
 * @param downlink_tbl 下行数据处理回调接口表，包含属性和服务的处理接口
 * @return 0 - 成功；-1 - 失败
 */
int32_t tm_init(struct tm_downlink_tbl_t *downlink_tbl);
int32_t tm_deinit(void);

/**
 * @brief 向平台发起设备登录请求
 *
 * @param product_id 产品ID
 * @param dev_name 设备名称
 * @param access_key 产品key或设备key
 * @param timeout_ms 登录超时时间
 * @return 0 - 登录成功；-1 - 失败
 */
int32_t tm_login(const uint8_t *product_id, const uint8_t *dev_name, const uint8_t *access_key, uint32_t timeout_ms);

/**
 * @brief 设备登出
 *
 * @param timeout_ms 超时时间
 * @return 0 - 登出成功；-1 - 失败
 */
int32_t tm_logout(uint32_t timeout_ms);

int32_t tm_post_property(void *prop_data, uint32_t timeout_ms);
int32_t tm_post_event(void *event_data, uint32_t timeout_ms);
int32_t tm_get_desired_props(uint32_t timeout_ms);
int32_t tm_delete_desired_props(uint32_t timeout_ms);

/**
 * @brief 打包设备的属性和事件数据，可用于子设备
 *
 * @param data 需要打包的目标指针地址，用于后续调用上报接口。设置为空时由接口内部分配空间，并通过返回值返回地址
 * @param product_id 需要打包数据的产品id
 * @param dev_name 需要打包数据的设备名称
 * @param prop
 * 传入属性数据。支持json格式(as_raw为1)，也可以仿照tm_user文件的数据上传接口使用tm_data接口构造数据（as_raw为0）
 * @param event 定义同prop，用于传入事件数据
 * @param as_raw 是否为原始json格式的数据
 * @return 打包后的数据指针地址
 */
void *  tm_pack_device_data(void *data, const uint8_t *product_id, const uint8_t *dev_name, void *prop, void *event,
                            int8_t as_raw);
int32_t tm_post_pack_data(void *pack_data, uint32_t timeout_ms);
int32_t tm_post_history_data(void *history_data, uint32_t timeout_ms);

#ifdef CONFIG_TM_GATEWAY
typedef int32_t (*tm_subdev_cb)(const uint8_t *name, void *data, uint32_t data_len);

int32_t tm_set_subdev_callback(tm_subdev_cb callback);

int32_t tm_post_raw(const uint8_t *name, uint8_t *raw_data, uint32_t raw_data_len, uint8_t **reply_data,
                    uint32_t *reply_data_len, uint32_t timeout_ms);
int32_t tm_send_request(const uint8_t *name, uint8_t as_raw, void *data, uint32_t data_len, void **reply_data,
                        uint32_t *reply_data_len, uint32_t timeout_ms);
int32_t tm_send_response(const uint8_t *name, uint8_t *msg_id, int32_t msg_code, uint8_t as_raw, void *resp_data,
                         uint32_t resp_data_len, uint32_t timeout_ms);
#endif
int32_t tm_step(uint32_t timeout_ms);

#ifdef CONFIG_TM_FMS
struct tm_file_data_t
{
    /** 本次获取的数据在文件数据的起始位置，从0开始*/
    uint32_t seq;
    /** 获取到的数据缓冲区地址*/
    uint8_t *data;
    /** 获取到的数据长度*/
    uint32_t data_len;
};

enum tm_file_event_e
{
    /** 获取文件数据回调，参数为struct tm_file_data_t*/
    TM_FILE_EVENT_GET_DATA = 0,
    /** 上传文件成功，参数为文件在平台保存的唯一ID，字符串型*/
    TM_FILE_EVENT_POST_SUCCESSED,
    /** 上传文件失败，参数为平台返回的错误信息，字符串型*/
    TM_FILE_EVENT_POST_FAILED,
};
typedef int32_t tm_file_cb(enum tm_file_event_e event, void *event_data);

/**
 * @brief 获取文件
 *
 * @param callback 文件事件回调接口
 * @param file_id 平台下发的文件唯一ID
 * @param file_size 需要获取的文件大小
 * @param segment_size 为节省终端资源，文件支持分片获取。本参数用于设置分片大小
 * @param timeout_ms 超时时间
 * @return 0 - 成功；其它 - 失败
 */
int32_t tm_file_get(tm_file_cb callback, uint8_t *file_id, uint32_t file_size, uint32_t segment_size,
                    uint32_t timeout_ms);

/**
 * @brief 向平台上传文件
 *
 * @param callback 文件事件回调接口
 * @param product_id 设备所属产品ID
 * @param dev_name 设备名称
 * @param access_key 产品key
 * @param file_name 文件名，具体限制参考平台说明文档
 * @param file 文件数据地址缓冲区
 * @param file_size 文件大小
 * @param timeout_ms 超时时间
 * @return 0 - 成功；其它 - 失败
 */
int32_t tm_file_post(tm_file_cb callback, const uint8_t *product_id, const uint8_t *dev_name, const uint8_t *access_key,
                     const uint8_t *file_name, uint8_t *file, uint32_t file_size, uint32_t timeout_ms);
#endif

#ifdef __cplusplus
}
#endif

#endif
