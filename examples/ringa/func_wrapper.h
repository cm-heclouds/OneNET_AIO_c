/**
 * Copyright (c), 2012~2021 iot.10086.cn All Rights Reserved
 * 
 * @file func_wrapper.h
 * @brief 
 */

#ifndef __FUNC_WRAPPER_H__
#define __FUNC_WRAPPER_H__

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
enum device_evt_id_t
{
    /** 远程复位*/
    DEVICE_EVT_ID_MODULE_RESET,
    /** FOTA升级请求*/
    DEVICE_EVT_ID_FOTA_REQUEST,
    /** SOTA升级请求*/
    DEVICE_EVT_ID_SOTA_REQUEST,
    /** 设备绑定*/
    DEVICE_EVT_ID_DEVICE_BIND,
    /** 设备解绑*/
    DEVICE_EVT_ID_DEVICE_UNBIND,
    DEVICE_EVT_ID_DUMMY = 0x7FFFFFFF
};

struct device_event_t
{
    enum device_evt_id_t evt_id;
    uint8_t *evt_data;
    uint32_t evt_data_len;
};

// ---------------------------------------------------------------------------------------
// |	         evt.evt_id     		|  evt.evt_data	|             返回值    			 |
// ---------------------------------------------------------------------------------------
// |           DEVICE_REGISTERED  		|      无       	|               0                |
// |        DEVICE_REGISTER_FAILED  	|      无       	|               0                |
// |        DEVICE_REGISTER_TIMEOUT  	|      无       	|               0                |
// |           ONENET_CONNECTED  		|      无       	|               0                |
// |         ONENET_DISCONNECTED 		|      无       	|               0                |
// |            FOTA_REQUEST   			|      无       	|0 - 不升级；其它 - 升级数据的最大分片 |
// |             FOTA_DATA     			|  参见串口协议   	|          参见串口协议            |
// ---------------------------------------------------------------------------------------
/**
 * 事件回调接口
 * @param evt 事件内容
 * @return 参见上表
 */
typedef int32_t (*device_event_cb)(struct device_event_t */** evt*/);

/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
void func_wrapper_init(device_event_cb evt_callback);

/**
 * 功能点数据上传接口
 * @param func_id 功能点ID
 * @param is_event 功能点类型：0:属性, 1:事件
 * @param timeout_ms 超时时间（毫秒）
 * @return 参见err.h
 */
int32_t func_upload(uint16_t func_id, uint8_t is_event, uint32_t timeout_ms);

/**
 * 功能点数据上传接口（多功能点）
 * @param func_id 功能点ID数组（包含需要上传的功能点）
 * @param count 功能点ID数组成员个数
 * @param is_event 功能点类型：0:属性, 1:事件
 * @param timeout_ms 超时时间（毫秒）
 * @return 参见err.h
 */
int32_t func_mult_upload(uint16_t *func_id, uint16_t count, uint8_t is_event, uint32_t timeout_ms);


#ifdef __cplusplus
}
#endif

#endif
