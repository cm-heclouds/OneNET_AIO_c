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
#include "iot_mqtt.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
/** 心跳时间*/
#define CN_LIFE_TIME_MQTT 120

/** OneNET平台产品ID*/
#define CN_PRODUCT_ID_MQTT ""

/** 设备ID*/
#define CN_DEVICE_ID_MQTT ""

/** 设备的鉴权信息*/
#define CN_DEVICE_KEY_MQTT ""

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/
int32_t iot_mqtt_cb_func(enum iot_dev_event_e event, void *data, uint32_t data_len);

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
int32_t           ptread_exit_flag = 0;
struct iot_data_t data_notify_test = {0};

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
int32_t iot_mqtt_cb_func(enum iot_dev_event_e event, void *data, uint32_t data_len)
{
    switch (event)
    {
        case IOT_DEV_CMD_RECEIVED:
        {
            log_debug("mqtt cmd arrived!\n");
            break;
        }
        case IOT_DEV_RESOURCE_PUSHED:
        {
            log_debug("mqtt notify ok\n");
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
    struct iot_cloud_config_t mqtt_config = {0};

    handle_t auto_notify_timer_handle = 0;
    uint32_t auto_notify_period_s     = 2;

    mqtt_config.product_id = (uint8_t *)CN_PRODUCT_ID_MQTT;
    mqtt_config.device_sn  = (uint8_t *)CN_DEVICE_ID_MQTT;
    mqtt_config.auth_code  = (uint8_t *)CN_DEVICE_KEY_MQTT;
    mqtt_config.life_time  = CN_LIFE_TIME_MQTT;

    /** 打开设备*/
    iot_dev_ptr = iot_dev_open(IOT_PROTO_MQTT, &mqtt_config, &iot_mqtt_cb_func, 60000);

    if (iot_dev_ptr != NULL)
    {
        /** 订阅资源*/
        log_debug("mqtt iot dev open sucessed!\n");
        iot_dev_observe(iot_dev_ptr, (const uint8_t *)"test_observe_data", 1000);
    }
    else
    {
        log_debug("mqtt iot dev open failed\n");
        return 0;
    }

    /** 初始化上传的数据*/
    data_notify_test.data_id     = (const uint8_t *)"test_data_stream";
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
        if (auto_notify_timer_handle == 0)
        {
            auto_notify_timer_handle = countdown_start(auto_notify_period_s * 1000);
        }
        else if (countdown_is_expired(auto_notify_timer_handle) == 1)
        {
            /** 上报数据到OneNET*/
            iot_dev_notify(iot_dev_ptr, &data_notify_test, 10000);

            data_notify_test.buf.val_int += 2;

            countdown_set(auto_notify_timer_handle, auto_notify_period_s * 1000);
        }

        time_delay_ms(50);
    }

    /** 关闭设备*/
    iot_dev_close(iot_dev_ptr, 3000);
    log_debug("mqtt test device closed!\n");

    return 0;
}
