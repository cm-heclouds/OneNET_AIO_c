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
#include "config.h"
#include "log.h"
#include "iot_api.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
/** 心跳时间*/
#define CN_LIFE_TIME_MQTTS      120 

/** OneNET平台产品ID*/  
#define CN_PRODUCT_ID_MQTTS     "260398"  

/** 设备唯一识别码，产品内唯一*/
#define CN_DEVICE_NAME_MQTTS    "gateway_001"       

/** 认证信息--设备key*/
#define CN_ACCESS_KEY_MQTTS     "bl1U7V5f7pU1O3teH71YlMefAbBFTYNAo6Cl0v210o8="

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/
int32_t iot_mqtts_cb_func(enum iot_dev_event_e event, void *data, uint32_t data_len);
/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
struct iot_data_t data_notify_list[] = 
{
    {.data_id = "data_stream_01", .data_type = IOT_DATA_TYPE_INT, .buf.val_int = 0},
    {.data_id = "data_stream_02", .data_type = IOT_DATA_TYPE_INT, .buf.val_int = 1},
    {.data_id = "data_stream_03", .data_type = IOT_DATA_TYPE_INT, .buf.val_int = 2},
};
int32_t data_notify_num = sizeof(data_notify_list)/sizeof(struct iot_data_t);
/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
int32_t iot_mqtts_cb_func(enum iot_dev_event_e event, void *data, uint32_t data_len)
{
    struct iot_dev_cmd_t *cmd_data_ptr = (struct iot_dev_cmd_t *)data;
    
    switch(event)
    {
        case IOT_DEV_CMD_RECEIVED:
        {
            /** 这里回环回去，测试下发命令是否成功*/
            osl_memcpy(cmd_data_ptr->resp, cmd_data_ptr->data, cmd_data_ptr->data_len);
            cmd_data_ptr->resp_len = cmd_data_ptr->data_len;
            log_debug("mqtts cmd arrived!\r\n");
            break;
        }
        case IOT_DEV_RESOURCE_PUSHED:
        {
            log_debug("mqtts other topic cmd received!\r\n");
            break;
        }
        default:
            break;
    }
    return 0;
}

int main(int arg, char *argv[])
{
    void *iot_dev_ptr = NULL;
    int32_t loop;
    handle_t timer_handle = 0;
    uint32_t timer_period = 2;
    struct iot_cloud_config_t mqtts_config = 
    {
        .product_id = CN_PRODUCT_ID_MQTTS,
        .device_sn  = CN_DEVICE_NAME_MQTTS,
        .auth_code  = CN_ACCESS_KEY_MQTTS,
        .life_time  = CN_LIFE_TIME_MQTTS,
    };

    /** 打开设备*/
    iot_dev_ptr = iot_dev_open(IOT_PROTO_MQTTS, &mqtts_config, &iot_mqtts_cb_func, 60000);
    
    if(iot_dev_ptr != NULL)
    {
        log_debug("mqtts iot dev open sucessed!");
        iot_dev_observe(iot_dev_ptr, "$sys/260398/gateway_001/cmd/#", 1000);
    }
    else
    {
        log_debug("mqtts iot dev open failed, err_code:%d", (int32_t)iot_dev_ptr);
        goto exit;
    }

    /** 设备处理主循环*/
    while(1)
    {
        /** 循环处理，包括心跳*/
        if(0 != iot_dev_step(iot_dev_ptr, 2000))
        {
            break;
        }

        /** 定时上报数据示例*/
        if(timer_handle == 0)
        {
            timer_handle = countdown_start(timer_period * 1000);
        }
        else if(countdown_is_expired(timer_handle) == 1)
        {
            /** 上报数据到OneNET*/
            for(loop=0; loop<data_notify_num; loop++)
            {
                data_notify_list[loop].buf.val_int+=2;
                if (0 == iot_dev_notify(iot_dev_ptr, &data_notify_list[loop], 10000))
                {
                    log_debug("mqtts notify ok\r\n");
                }
            }
            
            countdown_set(timer_handle, timer_period * 1000);
        }
        
        time_delay_ms(50);
    }
    
exit:    
    /** 关闭设备*/
    iot_dev_close(iot_dev_ptr, 3000);
    log_debug("mqtts test device closed!");
    
    return 0;
}
