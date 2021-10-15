/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file iot_mqtt.c
 * @date 2020/01/07
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "err_def.h"
#include "plat_osl.h"

#include "mqtt_api.h"

#include "iot_mqtt.h"
#include "onenet_payload.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define IOT_MQTT_SERVER_ADDR "183.230.40.39"
#define IOT_MQTT_SERVER_PORT 6002

#define IOT_MQTT_DS_TOPIC  "$dp"
#define IOT_MQTT_CMD_TOPIC "$creq/+"

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
struct iot_mqtt_t
{
    void *           client;
    iot_dev_event_cb evt_cb;
    int32_t          cmd_resp_len;
    uint8_t *        cmd_resp_topic;
    void *           send_buf;
    void *           recv_buf;
};
/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
static void iot_mqtt_message_arrived(void *arg, const uint8_t *topic, struct mqtt_message_t *message)
{
    struct iot_mqtt_t *  mqtt_dev = (struct iot_mqtt_t *)arg;
    uint8_t *            tmp_ptr  = NULL;
    struct iot_dev_cmd_t dev_cmd  = {0};

    if (NULL != (tmp_ptr = osl_strstr(topic, (const uint8_t *)"$creq/")))
    {
        dev_cmd.data_id  = tmp_ptr + 6;
        dev_cmd.data     = message->payload;
        dev_cmd.data_len = message->payload_len;
        dev_cmd.resp     = mqtt_dev->send_buf;
        dev_cmd.resp_len = 0;
        if (ERR_OK == mqtt_dev->evt_cb(IOT_DEV_CMD_RECEIVED, &dev_cmd, sizeof(dev_cmd)))
        {
            mqtt_dev->cmd_resp_len   = dev_cmd.resp_len;
            mqtt_dev->cmd_resp_topic = osl_strdup(topic);
            osl_memcpy((void *)mqtt_dev->cmd_resp_topic, (const void *)"$crsp", 5);
        }
        else
        {
            mqtt_dev->cmd_resp_len = -1;
        }
    }
    else
    {
        dev_cmd.data_id  = topic;
        dev_cmd.data     = message->payload;
        dev_cmd.data_len = message->payload_len;
        mqtt_dev->evt_cb(IOT_DEV_RESOURCE_PUSHED, &dev_cmd, sizeof(dev_cmd));
    }
}

void *iot_mqtt_open(struct iot_cloud_config_t *cloud, iot_dev_event_cb event_cb, uint32_t timeout_ms)
{
    struct iot_mqtt_t * mqtt_dev   = NULL;
    struct mqtt_param_t mqtt_param = {0};

    mqtt_dev = osl_malloc(sizeof(*mqtt_dev));

    if (mqtt_dev)
    {
        osl_memset(mqtt_dev, 0, sizeof(*mqtt_dev));
        mqtt_dev->send_buf = osl_malloc(CONFIG_SEND_BUF_LEN_MAX);
        mqtt_dev->recv_buf = osl_malloc(CONFIG_RECV_BUF_LEN_MAX);

        mqtt_param.client_id = cloud->device_sn;
        mqtt_param.username  = cloud->product_id;
        mqtt_param.password  = cloud->auth_code;
        mqtt_param.life_time = cloud->life_time;
        mqtt_param.connect_flag =
            MQTT_CONNECT_FLAG_CLEAN_SESSION | MQTT_CONNECT_FLAG_USERNAME | MQTT_CONNECT_FLAG_PASSWORD;
        mqtt_param.send_buf_len = CONFIG_SEND_BUF_LEN_MAX;
        mqtt_param.send_buf     = mqtt_dev->send_buf;
        mqtt_param.recv_buf_len = CONFIG_RECV_BUF_LEN_MAX;
        mqtt_param.recv_buf     = mqtt_dev->recv_buf;
        if (cloud->server_param)
        {
            mqtt_dev->client = mqtt_connect(cloud->server_param->remote_addr, cloud->server_param->remote_port,
                                            cloud->server_param->cert_buf.ca_cert, cloud->server_param->cert_len,
                                            &mqtt_param, timeout_ms);
        }
        else
        {
            mqtt_dev->client =
                mqtt_connect((const uint8_t *)IOT_MQTT_SERVER_ADDR, IOT_MQTT_SERVER_PORT, NULL, 0, &mqtt_param, timeout_ms);
        }

        if (NULL == mqtt_dev->client)
        {
            goto exit;
        }

        mqtt_set_default_message_handler(mqtt_dev->client, iot_mqtt_message_arrived, mqtt_dev);
        mqtt_dev->evt_cb       = event_cb;
        mqtt_dev->cmd_resp_len = -1;
    }
    return mqtt_dev;

exit:
    osl_free(mqtt_dev->send_buf);
    osl_free(mqtt_dev->recv_buf);
    osl_free(mqtt_dev);
    return NULL;
}

#if 0
int32_t iot_mqtt_get_info(void *dev, struct iot_dev_info_t *dev_info)
{

}
#endif

int32_t iot_mqtt_step(void *dev, uint32_t timeout_ms)
{
    struct iot_mqtt_t *mqtt_dev = (struct iot_mqtt_t *)dev;
    int32_t            ret      = mqtt_yield(mqtt_dev->client, timeout_ms);

    if ((ERR_OK == ret) && (0 <= mqtt_dev->cmd_resp_len))
    {
        uint8_t *resp_data = NULL;
        if (mqtt_dev->cmd_resp_len)
        {
            resp_data = osl_malloc(mqtt_dev->cmd_resp_len);
            osl_memcpy(resp_data, mqtt_dev->send_buf, mqtt_dev->cmd_resp_len);
        }
        ret = iot_mqtt_push_rawdata(mqtt_dev, mqtt_dev->cmd_resp_topic, resp_data, mqtt_dev->cmd_resp_len, timeout_ms);
        osl_free(mqtt_dev->cmd_resp_topic);
        if (resp_data)
            osl_free(resp_data);
        mqtt_dev->cmd_resp_len = -1;
    }
    return ret;
}

int32_t iot_mqtt_push_rawdata(void *dev, const uint8_t *target, void *raw_data, uint32_t data_len, uint32_t timeout_ms)
{
    struct iot_mqtt_t *   mqtt_dev = (struct iot_mqtt_t *)dev;
    struct mqtt_message_t msg;

    osl_memset(&msg, 0, sizeof(msg));
    msg.payload     = raw_data;
    msg.payload_len = data_len;

    return mqtt_publish(mqtt_dev->client, target, &msg, timeout_ms);
}

int32_t iot_mqtt_notify(void *dev, struct iot_data_t *data, uint32_t timeout_ms)
{
    struct iot_mqtt_t *mqtt_dev     = (struct iot_mqtt_t *)dev;
    uint8_t *          raw_data     = NULL;
    uint32_t           raw_data_len = 0;
    int32_t            ret          = 0;

    /** 根据iot_data_t构造payload数据*/
    if (NULL == (raw_data = osl_malloc(CONFIG_PACKET_PAYLOAD_LEN_MAX)))
    {
        return -1;
    }

    raw_data_len = iot_data_to_payload(raw_data, data);

    ret = iot_mqtt_push_rawdata(mqtt_dev, (const uint8_t *)IOT_MQTT_DS_TOPIC, raw_data, raw_data_len, timeout_ms);
    osl_free(raw_data);
    return ret;
}

int32_t iot_mqtt_observe(void *dev, const uint8_t *resource, uint32_t timeout_ms)
{
    struct iot_mqtt_t *mqtt_dev = (struct iot_mqtt_t *)dev;

    return mqtt_subscribe(mqtt_dev->client, resource, MQTT_QOS1, iot_mqtt_message_arrived, dev, timeout_ms);
}

int32_t iot_mqtt_observe_cancel(void *dev, const uint8_t *resource, uint32_t timeout_ms)
{
    struct iot_mqtt_t *mqtt_dev = (struct iot_mqtt_t *)dev;

    return mqtt_unsubscribe(mqtt_dev->client, resource, timeout_ms);
}

int32_t iot_mqtt_close(void *dev, uint32_t timeout_ms)
{
    struct iot_mqtt_t *mqtt_dev = (struct iot_mqtt_t *)dev;

    mqtt_disconnect(mqtt_dev->client, timeout_ms);
    osl_free(mqtt_dev->recv_buf);
    osl_free(mqtt_dev->send_buf);
    osl_free(mqtt_dev);

    return 0;
}
