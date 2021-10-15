/**
 * Copyright (c), 2012~2021 iot.10086.cn All Rights Reserved
 * 
 * @file func_wrapper.c
 * @brief 
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "func_wrapper.h"
#include "ringa_api.h"
#include "ringa_cmd.h"
#include "ringa_tlv.h"

#include "func_ops.h"

#include "plat_osl.h"
#include "err_def.h"
#include "config.h"
/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
device_event_cb dev_evt_callback;

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
static int32_t device_property_read(uint16_t func_id, uint8_t *func_data, uint16_t *func_data_len)
{
    int32_t ret = func_get_val(func_id, func_data, *func_data_len, FUNC_LEVEL_QUERY);

    if(0 < ret)
    {
        *func_data_len = ret;
        return ERR_OK;
    }
    else
    {
        return ERR_INVALID_DATA;
    }
}

static int32_t device_property_write(uint16_t func_id, uint8_t *func_data, uint16_t func_data_len)
{
    return func_set_val(func_id, func_data, func_data_len);
}

static int32_t ringa_cmd_handle(const uint8_t *cmd_id, void *cmd_data, uint32_t cmd_data_len)
{
    int32_t ret = 0;
    uint32_t parsed_len = 0;
    uint32_t cmd = 0;
    enum ringa_tlv_type_e type = 0;
    uint8_t *data = NULL;
    uint16_t data_len = 0;
    boolean resp_code = 0;
    uint8_t *resp_data = NULL;
    uint16_t resp_data_len = 4;
    uint16_t offset = 0;

    if (NULL == (resp_data = osl_malloc(CONFIG_PACKET_PAYLOAD_LEN_MAX)))
    {
        return ERR_NO_MEM;
    }
    osl_memset(resp_data, 0, CONFIG_PACKET_PAYLOAD_LEN_MAX);

    do
    {
        parsed_len += ringa_cmd_parse(cmd_data + parsed_len, &cmd, &type, &data, &data_len);
        switch(cmd >> 16)
        {
            case RINGA_CMD_TYPE_SYSTEM:
            {
                struct device_event_t event = {0};

                switch(cmd & 0xFF)
                {
                    case RINGA_CMD_SYSTEM_BIND:
                    {
                        event.evt_id = DEVICE_EVT_ID_DEVICE_BIND;
                    }
                    break;
                    case RINGA_CMD_SYSTEM_UNBIND:
                    {
                        event.evt_id = DEVICE_EVT_ID_DEVICE_UNBIND;
                    }
                    break;
                    case RINGA_CMD_SYSTEM_UPGRADE:
                    {
                        event.evt_id = DEVICE_EVT_ID_FOTA_REQUEST;
                    }
                    break;
                }
                ret = dev_evt_callback(&event);
            }
            break;
            case RINGA_CMD_TYPE_PROPERTY_WIRTE:
            {
                ret = device_property_write(cmd & 0xFF, data, data_len);
            }
            break;
            case RINGA_CMD_TYPE_PROPERTY_READ:
            {
                ret = device_property_read(cmd & 0xFF, resp_data + resp_data_len, &offset);
                if(ERR_OK == ret)
                {
                    resp_data_len += offset;
                }
                else
                {
                    resp_data_len = 4;
                    break;
                }
            }
            break;
            default: break;
        }
    } while((parsed_len < cmd_data_len) && (ERR_OK == ret));

    resp_code = (ERR_OK == ret) ? TRUE : FALSE;
    ringa_tlv_ser(resp_data, RINGA_TLV_BOOLEAN, (uint8_t *)&resp_code, sizeof(resp_code));

    ret = ringa_cmd_resp(cmd_id, resp_data, resp_data_len, CONFIG_DEFAULT_REQUEST_TIMEOUT);
    osl_free(resp_data);

    return ret;
}

void func_wrapper_init(device_event_cb evt_callback)
{
    ringa_set_callback(ringa_cmd_handle);
    dev_evt_callback = evt_callback;
}

/**
 * 功能点数据上传接口
 * @param func_id 功能点ID
 * @param is_event 功能点类型：0:属性, 1:事件
 * @param timeout_ms 超时时间（毫秒）
 * @return 参见err.h
 */
int32_t func_upload(uint16_t func_id, uint8_t is_event, uint32_t timeout_ms)
{
    uint8_t *payload = NULL;
    uint32_t payload_len = 0;
    int32_t ret;

    if(NULL == (payload = osl_malloc(CONFIG_PACKET_PAYLOAD_LEN_MAX)))
    {
        return ERR_NO_MEM;
    }
    payload_len =
        func_get_val(func_id, payload, CONFIG_PACKET_PAYLOAD_LEN_MAX, is_event ? FUNC_LEVEL_EVENT : FUNC_LEVEL_UPLOAD);

    ret = ringa_notify(payload, payload_len, NULL, timeout_ms);
    osl_free(payload);

    return ret;
}

/**
 * 功能点数据上传接口（多功能点）
 * @param func_id 功能点ID数组（包含需要上传的功能点）
 * @param count 功能点ID数组成员个数
 * @param is_event 功能点类型：0:属性, 1:事件
 * @param timeout_ms 超时时间（毫秒）
 * @return 参见err.h
 */
int32_t func_mult_upload(uint16_t *func_id, uint16_t count, uint8_t is_event, uint32_t timeout_ms)
{
    uint8_t *payload = NULL;
    uint32_t payload_len = 0;
    int32_t i = 0;
    int32_t ret;

    if(NULL == (payload = osl_malloc(CONFIG_PACKET_PAYLOAD_LEN_MAX)))
    {
        return ERR_NO_MEM;
    }
    for(i = 0; i < count; i++)
    {
        payload_len = func_get_val(func_id[i], payload + payload_len, CONFIG_PACKET_PAYLOAD_LEN_MAX - payload_len,
                                   is_event ? FUNC_LEVEL_EVENT : FUNC_LEVEL_UPLOAD);
    }

    ret = ringa_notify(payload, payload_len, NULL, timeout_ms);
    osl_free(payload);

    return ret;
}
