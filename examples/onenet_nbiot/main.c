/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file main.c
 * @date 2020/06/04
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "plat_time.h"
#include "plat_osl.h"
#include "iot_api.h"
#include "log.h"
#include "nbiot_m2m.h"
#include "utils.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
/** 心跳时间*/
#define CN_LIFE_TIME_NBIOT 120

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/
int32_t nbiot_cb_func(enum iot_dev_event_e event, void *data, uint32_t data_len);

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static int8_t iot_dev_cloud_connected = 0;

/** 定义资源模型*/
struct iot_data_t resource_list[] = {
    {.data_id = (const uint8_t *)"3311_0_5850", .data_type = IOT_DATA_TYPE_INT, .buf.val_int = 0},
    {.data_id = (const uint8_t *)"3311_0_5851", .data_type = IOT_DATA_TYPE_INT, .buf.val_int = 60},
    {.data_id = (const uint8_t *)"3311_1_5850", .data_type = IOT_DATA_TYPE_INT, .buf.val_int = 1},
    {.data_id = (const uint8_t *)"3311_1_5851", .data_type = IOT_DATA_TYPE_INT, .buf.val_int = 80},
    {.data_id = (const uint8_t *)"3303_0_5700", .data_type = IOT_DATA_TYPE_FLOAT, .buf.val_float = 57.12},
    {.data_id     = (const uint8_t *)"3303_0_5701",
     .data_type   = IOT_DATA_TYPE_STRING,
     .buf.val_str = (uint8_t *)"sheshidu"},
};
int32_t resource_num = sizeof(resource_list) / sizeof(struct iot_data_t);

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
static int32_t iot_dev_m2m_read(struct m2m_data_t *data_info, struct iot_data_t *data)
{
    switch (data_info->data_type)
    {
        case M2M_DT_BOOL:
        case M2M_DT_INTEGER:
            data_info->data_type    = data->data_type;
            data_info->data.val_int = data->buf.val_int;
            break;
        case M2M_DT_FLOAT:
            data_info->data_type      = data->data_type;
            data_info->data.val_float = data->buf.val_float;
            break;
        case M2M_DT_STRING:
            data_info->data_type    = data->data_type;
            data_info->data.val_str = data->buf.val_str;
            break;
        case M2M_DT_OPAQUE:
            data_info->data_type             = data->data_type;
            data_info->data.val_opa.data     = data->buf.val_bin.buf;
            data_info->data.val_opa.data_len = data->buf.val_bin.buf_len;
            break;
        default:
            break;
    }

    return 1;
}

static int32_t iot_dev_m2m_write(struct m2m_data_t *data_info, struct iot_data_t *data)
{
    int32_t ret = 2;
    switch (data_info->data_type)
    {
        case M2M_DT_BOOL:
        case M2M_DT_INTEGER:
            data->buf.val_int = data_info->data.val_int;
            break;
        case M2M_DT_FLOAT:
            data->buf.val_float = data_info->data.val_float;
            break;
        case M2M_DT_STRING:
            osl_memset(data->buf.val_str, 0, osl_strlen(data->buf.val_str));
            osl_memcpy(data->buf.val_str, data_info->data.val_str, osl_strlen(data_info->data.val_str));
            break;
        case M2M_DT_OPAQUE:
            osl_memset(data->buf.val_bin.buf, 0, data->buf.val_bin.buf_len);
            data->buf.val_bin.buf_len = data_info->data.val_opa.data_len;
            osl_memcpy(data->buf.val_bin.buf, data_info->data.val_opa.data, data->buf.val_bin.buf_len);
            break;
        default:
            ret = 11;
            break;
    }

    return ret;
}

int32_t nbiot_cb_func(enum iot_dev_event_e event, void *data, uint32_t data_len)
{
    switch (event)
    {
        case IOT_DEV_CLOUD_CONNECTED:
        {
            iot_dev_cloud_connected = 1;
            break;
        }
        case IOT_DEV_CMD_RECEIVED:
        {
            int8_t             loop;
            uint8_t            data_id[32]  = {0};
            struct m2m_attr_t *m2m_attr_ptr = (struct m2m_attr_t *)data;
            osl_sprintf(data_id, (const uint8_t *)"%d_%d_%d", m2m_attr_ptr->data_info.res_info.obj_id,
                        m2m_attr_ptr->data_info.res_info.inst_id, m2m_attr_ptr->data_info.res_info.res_id);
            switch (m2m_attr_ptr->attr_type)
            {
                case M2M_ATTR_READ:
                {
                    for (loop = 0; loop < resource_num; loop++)
                    {
                        if (0 == osl_strcmp(resource_list[loop].data_id, data_id))
                        {
                            resource_list[loop].buf.val_int += 1; /** 可以改变读时候的值*/
                            m2m_attr_ptr->attr_result =
                                iot_dev_m2m_read(&m2m_attr_ptr->data_info, &resource_list[loop]);
                        }
                    }
                    break;
                }
                case M2M_ATTR_WRITE:
                {
                    for (loop = 0; loop < resource_num; loop++)
                    {
                        if (0 == osl_strcmp(resource_list[loop].data_id, data_id))
                        {
                            m2m_attr_ptr->attr_result =
                                iot_dev_m2m_write(&m2m_attr_ptr->data_info, &resource_list[loop]);
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case IOT_DEV_RESOURCE_PUSHED:
        {
            log_debug("notify ok!\r\n");
            break;
        }
        default:
            break;
    }
    return 0;
}

int main(int arg, char *argv[])
{
    void *                    iot_dev_ptr  = NULL;
    struct iot_cloud_config_t nbiot_config = {0};
    handle_t                  timer_handle = 0;
    uint32_t                  timer_period = 5;
    int8_t                    loop;

    /** 初始化配置信息*/
    nbiot_config.life_time = CN_LIFE_TIME_NBIOT;

    /** 打开设备*/
    iot_dev_ptr = iot_dev_open(IOT_PROTO_NBIOT, &nbiot_config, &nbiot_cb_func, 60000);

    if (!iot_dev_ptr)
    {
        return 0;
    }

    /** 订阅资源*/
    for (loop = 0; loop < resource_num; loop++)
    {
        iot_dev_observe(iot_dev_ptr, resource_list[loop].data_id, 1000);
    }

    while (1)
    {
        /** 循环处理*/
        iot_dev_step(iot_dev_ptr, 2000);

        if (iot_dev_cloud_connected)
        {
            /** 定时上报数据示例*/
            if (timer_handle == 0)
            {
                timer_handle = countdown_start(timer_period * 1000);
            }
            else if (countdown_is_expired(timer_handle) == 1)
            {
                /** 上报数据到OneNET*/
                for (loop = 0; loop < resource_num; loop++)
                {
                    iot_dev_notify(iot_dev_ptr, &resource_list[loop], 10000);
                }
                countdown_set(timer_handle, timer_period * 1000);
            }
        }
        time_delay_ms(200);
    }

    /** 关闭设备*/
    iot_dev_close(iot_dev_ptr, 3000);
    log_debug("nbiot test device closed!\n");

    return 0;
}