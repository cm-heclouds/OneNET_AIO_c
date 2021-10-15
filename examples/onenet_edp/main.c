/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file main.c
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "plat_time.h"
#include "plat_osl.h"
#include "log.h"

#include "iot_api.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
/** 心跳时间*/
#define CN_LIFE_TIME_EDP 120

/** 产品ID + 鉴权信息（auth_info）*/
#define CN_PRODUCT_ID_EDP ""
#define CN_DEVICE_ID_EDP  ""

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/
int32_t iot_edp_cb_func(enum iot_dev_event_e event, void *data, uint32_t data_len);

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
struct iot_data_t data_notify_test = {0};

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
int32_t iot_edp_cb_func(enum iot_dev_event_e event, void *data, uint32_t data_len)
{
    switch (event)
    {
        case IOT_DEV_CMD_RECEIVED:
        {
            log_debug("edp cmd arrived!\n");
            break;
        }
        case IOT_DEV_RESOURCE_PUSHED:
        {
            log_debug("edp get pushed!\n");
            break;
        }
        default:
            break;
    }
    return 0;
}

int main(int arg, char *argv[])
{
    void *                    iot_dev_ptr = NULL;
    struct iot_cloud_config_t edp_config  = {0};

    handle_t timer_handle = 0;
    uint32_t timer_period = 2;

    edp_config.product_id = CN_PRODUCT_ID_EDP;
    edp_config.device_sn  = CN_DEVICE_ID_EDP;
    edp_config.life_time  = CN_LIFE_TIME_EDP;

    /** 打开设备*/
    iot_dev_ptr = iot_dev_open(IOT_PROTO_EDP, &edp_config, &iot_edp_cb_func, 60000);

    if (iot_dev_ptr != NULL)
    {
        log_debug("edp iot dev open sucessed!\n");
    }
    else
    {
        log_debug("edp iot dev open failed!\n");
        goto exit;
    }

    /** 初始化上传的数据*/
    data_notify_test.data_id     = "test_data_stream";
    data_notify_test.data_type   = IOT_DATA_TYPE_INT;
    data_notify_test.buf.val_int = 99;

    /** 设备处理主循环*/
    while (1)
    {
        /** 循环处理，包括心跳*/
        if (0 != iot_dev_step(iot_dev_ptr, 2000))
        {
            break;
        }

        /** 定时上报数据示例*/
        if (timer_handle == 0)
        {
            timer_handle = countdown_start(timer_period * 1000);
        }
        else if (countdown_is_expired(timer_handle) == 1)
        {
            /** 上报数据到OneNET*/
            iot_dev_notify(iot_dev_ptr, &data_notify_test, 10000);
            log_debug("edp iot dev notify!\n");

            data_notify_test.buf.val_int += 2;

            countdown_set(timer_handle, timer_period * 1000);
        }

        time_delay_ms(50);
    }

exit:
    /** 关闭设备*/
    iot_dev_close(iot_dev_ptr, 3000);
    log_debug("edp test device closed!\n");

    return 0;
}
