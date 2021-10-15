/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file iot_mqtts.c
 * @date 2020/01/07
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "err_def.h"
#include "plat_osl.h"
#include "plat_time.h"
#include "mqtt_api.h"
#include "iot_mqtts.h"
#include "cJSON.h"
#include "dev_token.h"

#include "log.h"
/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#ifdef CONFIG_IOT_MQTTS_TLS
#define IOT_MQTTS_SERVER_ADDR_TLS "183.230.40.16"
#define IOT_MQTTS_SERVER_PORT_TLS 8883

static const uint8_t *mqtts_cert =
    (const uint8_t *)"-----BEGIN CERTIFICATE-----\r\n"
                     "MIIDOzCCAiOgAwIBAgIJAPCCNfxANtVEMA0GCSqGSIb3DQEBCwUAMDQxCzAJBgNV\r\n"
                     "BAYTAkNOMQ4wDAYDVQQKDAVDTUlPVDEVMBMGA1UEAwwMT25lTkVUIE1RVFRTMB4X\r\n"
                     "DTE5MDUyOTAxMDkyOFoXDTQ5MDUyMTAxMDkyOFowNDELMAkGA1UEBhMCQ04xDjAM\r\n"
                     "BgNVBAoMBUNNSU9UMRUwEwYDVQQDDAxPbmVORVQgTVFUVFMwggEiMA0GCSqGSIb3\r\n"
                     "DQEBAQUAA4IBDwAwggEKAoIBAQC/VvJ6lGWfy9PKdXKBdzY83OERB35AJhu+9jkx\r\n"
                     "5d4SOtZScTe93Xw9TSVRKrFwu5muGgPusyAlbQnFlZoTJBZY/745MG6aeli6plpR\r\n"
                     "r93G6qVN5VLoXAkvqKslLZlj6wXy70/e0GC0oMFzqSP0AY74icANk8dUFB2Q8usS\r\n"
                     "UseRafNBcYfqACzF/Wa+Fu/upBGwtl7wDLYZdCm3KNjZZZstvVB5DWGnqNX9HkTl\r\n"
                     "U9NBMS/7yph3XYU3mJqUZxryb8pHLVHazarNRppx1aoNroi+5/t3Fx/gEa6a5PoP\r\n"
                     "ouH35DbykmzvVE67GUGpAfZZtEFE1e0E/6IB84PE00llvy3pAgMBAAGjUDBOMB0G\r\n"
                     "A1UdDgQWBBTTi/q1F2iabqlS7yEoX1rbOsz5GDAfBgNVHSMEGDAWgBTTi/q1F2ia\r\n"
                     "bqlS7yEoX1rbOsz5GDAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQAL\r\n"
                     "aqJ2FgcKLBBHJ8VeNSuGV2cxVYH1JIaHnzL6SlE5q7MYVg+Ofbs2PRlTiWGMazC7\r\n"
                     "q5RKVj9zj0z/8i3ScWrWXFmyp85ZHfuo/DeK6HcbEXJEOfPDvyMPuhVBTzuBIRJb\r\n"
                     "41M27NdIVCdxP6562n6Vp0gbE8kN10q+ksw8YBoLFP0D1da7D5WnSV+nwEIP+F4a\r\n"
                     "3ZX80bNt6tRj9XY0gM68mI60WXrF/qYL+NUz+D3Lw9bgDSXxpSN8JGYBR85BxBvR\r\n"
                     "NNAhsJJ3yoAvbPUQ4m8J/CoVKKgcWymS1pvEHmF47pgzbbjm5bdthlIx+swdiGFa\r\n"
                     "WzdhzTYwVkxBaU+xf/2w\r\n"
                     "-----END CERTIFICATE-----";
#else
#define IOT_MQTTS_SERVER_ADDR "183.230.40.96"
#define IOT_MQTTS_SERVER_PORT 1883
#endif

#define MQTTS_DEFAULT_TOPIC_POST        "$sys/%s/%s/dp/post/json"          // pub
#define MQTTS_DEFAULT_TOPIC_POST_ACCEPT "$sys/%s/%s/dp/post/json/accepted" // sub
#define MQTTS_DEFAULT_TOPIC_POST_REJECT "$sys/%s/%s/dp/post/json/rejected" // sub

#define MQTTS_DEFAULT_TOPIC_CMD_REQUEST  "$sys/%s/%s/cmd/request/+"  // sub
#define MQTTS_DEFAULT_TOPIC_CMD_RESPONSE "$sys/%s/%s/cmd/response/+" // pub

#define MQTTS_DEFAULT_SUBED_TOPIC "$sys/%s/%s/#"

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
struct default_topic_t
{
    uint8_t *post;
    uint8_t *cmd_resp;
    uint8_t *subed;
};

struct iot_mqtts_t
{
    void *                 client;
    uint8_t *              send_buf;
    uint8_t *              recv_buf;
    uint32_t               msg_id;
    iot_dev_event_cb       evt_cb;
    struct default_topic_t topic;
    uint32_t               reply_flag;
    uint32_t               reply_accept;
    uint32_t               reply_id;
    int32_t                cmd_resp_len;
    uint8_t *              cmd_resp_topic;
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
static uint32_t construct_payload(uint32_t msg_id, uint8_t *payload, struct iot_data_t *data)
{
    cJSON *  root      = NULL;
    cJSON *  val_array = NULL;
    cJSON *  val_item  = NULL;
    cJSON *  dp_item   = NULL;
    uint8_t *temp_str  = NULL;

    if (NULL == (root = cJSON_CreateObject()))
    {
        return ERR_OK;
    }

    cJSON_AddNumberToObject(root, "id", msg_id);

    if (NULL == (val_array = cJSON_CreateArray()))
    {
        goto exit;
    }

    if (NULL == (val_item = cJSON_CreateObject()))
    {
        goto exit1;
    }
    if (NULL == (dp_item = cJSON_CreateObject()))
    {
        goto exit2;
    }

    switch (data->data_type)
    {
        case IOT_DATA_TYPE_INT:
        {
            cJSON_AddNumberToObject(val_item, "v", data->buf.val_int);
            break;
        }
        case IOT_DATA_TYPE_FLOAT:
        {
            cJSON_AddNumberToObject(val_item, "v", data->buf.val_float);
            break;
        }
        case IOT_DATA_TYPE_STRING:
        {
            cJSON_AddStringToObject(val_item, "v", (const char *const)data->buf.val_str);
            break;
        }
        default:
            break;
    }
    if (data->timestamp)
    {
        cJSON_AddNumberToObject(val_item, "t", data->timestamp);
    }
    cJSON_AddItemToArray(val_array, val_item);
    cJSON_AddItemToObject(dp_item, (const char *)data->data_id, val_array);
    cJSON_AddItemToObject(root, "dp", dp_item);

    if (NULL != (temp_str = (uint8_t *)cJSON_PrintUnformatted(root)))
    {
        osl_strcpy(payload, temp_str);
        osl_free(temp_str);
    }
    cJSON_Delete(root);

    return osl_strlen(payload);

exit2:
    cJSON_Delete(val_item);
exit1:
    cJSON_Delete(val_array);
exit:
    cJSON_Delete(root);
    return 0;
}

static void iot_mqtts_message_arrived(void *arg, const uint8_t *topic, struct mqtt_message_t *message)
{
    struct iot_mqtts_t * mqtts_dev = (struct iot_mqtts_t *)arg;
    struct iot_dev_cmd_t cmd_data  = {0};
    uint8_t *            tmp_ptr   = NULL;

    if (NULL != (tmp_ptr = osl_strstr(topic, (const uint8_t *)"$sys")))
    {
        if (NULL != (tmp_ptr = osl_strstr(topic, (const uint8_t *)"/cmd/request/"))) // data cmd
        {
            cmd_data.data_id  = tmp_ptr + 13;
            cmd_data.data     = message->payload;
            cmd_data.data_len = message->payload_len;
            cmd_data.resp     = mqtts_dev->send_buf;
            cmd_data.resp_len = 0;
            if (ERR_OK == mqtts_dev->evt_cb(IOT_DEV_CMD_RECEIVED, &cmd_data, sizeof(struct iot_dev_cmd_t)))
            {
                mqtts_dev->cmd_resp_len   = cmd_data.resp_len;
                mqtts_dev->cmd_resp_topic = osl_malloc(osl_strlen(topic) + 2);
                osl_memset(mqtts_dev->cmd_resp_topic, 0, osl_strlen(topic) + 2);
                osl_memcpy(mqtts_dev->cmd_resp_topic, topic, osl_strlen(topic) - osl_strlen(cmd_data.data_id) - 8);
                osl_strcat(mqtts_dev->cmd_resp_topic, (const uint8_t *)"response/");
                osl_strcat(mqtts_dev->cmd_resp_topic, cmd_data.data_id);
            }
            else
            {
                mqtts_dev->cmd_resp_len = -1;
            }
        }
        else if (NULL != (tmp_ptr = osl_strstr(topic, (const uint8_t *)"/post/json/"))) // data notify reply
        {
            cJSON *reply = cJSON_ParseWithLength((const char *)message->payload, message->payload_len);
            if (osl_strstr(tmp_ptr, (const uint8_t *)"accepted"))
            {
                mqtts_dev->reply_accept = 1;
            }
            else
            {
                mqtts_dev->reply_accept = 0;
            }
            mqtts_dev->reply_id = cJSON_GetObjectItem(reply, (const char *const)"id")->valueint;
            cJSON_Delete(reply);
            mqtts_dev->reply_flag = 1;
        }
    }
    else
    {
        cmd_data.data_id  = topic;
        cmd_data.data     = message->payload;
        cmd_data.data_len = message->payload_len;
        mqtts_dev->evt_cb(IOT_DEV_RESOURCE_PUSHED, &cmd_data, sizeof(struct iot_dev_cmd_t));
    }
}

static uint8_t *iot_mqtts_topic_construct(const uint8_t *format, const uint8_t *pid, const uint8_t *dev_name)
{
    uint32_t topic_len = 0;
    uint8_t *topic     = NULL;

    topic_len = osl_strlen(format) + osl_strlen(pid) + osl_strlen(dev_name) - 3;

    if (NULL != (topic = osl_malloc(topic_len)))
    {
        osl_memset(topic, 0, topic_len);
        osl_sprintf(topic, format, pid, dev_name);
    }

    return topic;
}

static void iot_mqtts_topic_free(void *dev)
{
    struct iot_mqtts_t *mqtts_dev = (struct iot_mqtts_t *)dev;

    if (mqtts_dev->topic.post)
    {
        osl_free(mqtts_dev->topic.post);
    }
    if (mqtts_dev->topic.cmd_resp)
    {
        osl_free(mqtts_dev->topic.cmd_resp);
    }
    if (mqtts_dev->topic.subed)
    {
        osl_free(mqtts_dev->topic.subed);
    }
}

static void *iot_mqtts_init(void)
{
    struct iot_mqtts_t *mqtts_dev = NULL;

    if (NULL == (mqtts_dev = osl_malloc(sizeof(*mqtts_dev))))
    {
        goto exit1;
    }
    osl_memset(mqtts_dev, 0, sizeof(*mqtts_dev));

    if (NULL == (mqtts_dev->send_buf = osl_malloc(CONFIG_SEND_BUF_LEN_MAX)))
    {
        goto exit2;
    }

    if (NULL == (mqtts_dev->recv_buf = osl_malloc(CONFIG_RECV_BUF_LEN_MAX)))
    {
        goto exit3;
    }

    return mqtts_dev;

exit3:
    osl_free(mqtts_dev->send_buf);
exit2:
    osl_free(mqtts_dev);
exit1:
    return NULL;
}

static void iot_mqtts_deinit(void *dev)
{
    struct iot_mqtts_t *mqtts_dev = (struct iot_mqtts_t *)dev;

    if (mqtts_dev->send_buf)
    {
        osl_free(mqtts_dev->send_buf);
    }
    if (mqtts_dev->recv_buf)
    {
        osl_free(mqtts_dev->recv_buf);
    }
    if (mqtts_dev)
    {
        osl_free(mqtts_dev);
    }
}

void *iot_mqtts_open(struct iot_cloud_config_t *cloud, iot_dev_event_cb event_cb, uint32_t timeout_ms)
{
    struct iot_mqtts_t *mqtts_dev      = NULL;
    handle_t            cd_hdl         = 0;
    struct mqtt_param_t mqtt_param     = {0};
    uint8_t             dev_token[256] = {0};

    if (0 == (cd_hdl = countdown_start(timeout_ms)))
    {
        return NULL;
    }
    if (NULL == (mqtts_dev = (struct iot_mqtts_t *)iot_mqtts_init()))
    {
        goto exit1;
    }

    dev_token_generate(dev_token, SIG_METHOD_SHA1, 2524579200, cloud->product_id, cloud->device_sn, cloud->auth_code);
    log_debug("token = %s\n", dev_token);

    mqtt_param.client_id    = cloud->device_sn;
    mqtt_param.username     = cloud->product_id;
    mqtt_param.password     = dev_token;
    mqtt_param.life_time    = cloud->life_time;
    mqtt_param.connect_flag = MQTT_CONNECT_FLAG_CLEAN_SESSION | MQTT_CONNECT_FLAG_USERNAME | MQTT_CONNECT_FLAG_PASSWORD;

    mqtt_param.send_buf_len = CONFIG_SEND_BUF_LEN_MAX;
    mqtt_param.send_buf     = mqtts_dev->send_buf;
    mqtt_param.recv_buf_len = CONFIG_RECV_BUF_LEN_MAX;
    mqtt_param.recv_buf     = mqtts_dev->recv_buf;

    mqtts_dev->topic.post =
        iot_mqtts_topic_construct((const uint8_t *)MQTTS_DEFAULT_TOPIC_POST, cloud->product_id, cloud->device_sn);
    mqtts_dev->topic.subed =
        iot_mqtts_topic_construct((const uint8_t *)MQTTS_DEFAULT_SUBED_TOPIC, cloud->product_id, cloud->device_sn);
    mqtts_dev->topic.cmd_resp = iot_mqtts_topic_construct((const uint8_t *)MQTTS_DEFAULT_TOPIC_CMD_RESPONSE,
                                                          cloud->product_id, cloud->device_sn);

    if (cloud->server_param)
    {
        mqtts_dev->client =
            mqtt_connect(cloud->server_param->remote_addr, cloud->server_param->remote_port,
                         cloud->server_param->cert_buf.ca_cert, cloud->server_param->cert_len, &mqtt_param, timeout_ms);
    }
    else
    {
#ifdef CONFIG_IOT_MQTTS_TLS
        mqtts_dev->client = mqtt_connect((const uint8_t *)IOT_MQTTS_SERVER_ADDR_TLS, IOT_MQTTS_SERVER_PORT_TLS,
                                         mqtts_cert, osl_strlen(mqtts_cert), &mqtt_param, timeout_ms);
#else
        mqtts_dev->client = mqtt_connect((const uint8_t *)IOT_MQTTS_SERVER_ADDR, IOT_MQTTS_SERVER_PORT, NULL, 0,
                                         &mqtt_param, timeout_ms);
#endif
    }
    if (NULL == mqtts_dev->client)
    {
        goto exit2;
    }
    if (0
        != mqtt_subscribe(mqtts_dev->client, mqtts_dev->topic.subed, MQTT_QOS0, iot_mqtts_message_arrived, mqtts_dev,
                          countdown_left(cd_hdl)))
    {
        goto exit3;
    }
    mqtts_dev->evt_cb       = event_cb;
    mqtts_dev->cmd_resp_len = -1;
    countdown_stop(cd_hdl);
    return mqtts_dev;

exit3:
    mqtt_disconnect(mqtts_dev->client, timeout_ms);
exit2:
    iot_mqtts_topic_free(mqtts_dev);
    iot_mqtts_deinit(mqtts_dev);
exit1:
    countdown_stop(cd_hdl);
    return NULL;
}

#if 0
int32_t iot_mqtts_get_info(void *dev, struct iot_dev_info_t *dev_info)
{

}
#endif

int32_t iot_mqtts_step(void *dev, uint32_t timeout_ms)
{
    struct iot_mqtts_t *mqtts_dev = (struct iot_mqtts_t *)dev;
    int32_t             ret       = ERR_OK;

    ret = mqtt_yield(mqtts_dev->client, timeout_ms);
    if ((ERR_OK == ret) && (0 <= mqtts_dev->cmd_resp_len))
    {
        uint8_t *resp_data = NULL;
        if (mqtts_dev->cmd_resp_len)
        {
            resp_data = osl_malloc(mqtts_dev->cmd_resp_len);
            osl_memcpy(resp_data, mqtts_dev->send_buf, mqtts_dev->cmd_resp_len);
        }
        ret = iot_mqtts_push_rawdata(dev, mqtts_dev->cmd_resp_topic, resp_data, mqtts_dev->cmd_resp_len, timeout_ms);
        osl_free(mqtts_dev->cmd_resp_topic);
        if (resp_data)
            osl_free(resp_data);
        mqtts_dev->cmd_resp_len = -1;
    }

    return ret;
}

int32_t iot_mqtts_push_rawdata(void *dev, const uint8_t *target, void *raw_data, uint32_t data_len, uint32_t timeout_ms)
{
    struct iot_mqtts_t *  mqtts_dev = (struct iot_mqtts_t *)dev;
    struct mqtt_message_t msg;

    osl_memset(&msg, 0, sizeof(msg));
    msg.payload     = raw_data;
    msg.payload_len = data_len;

    return mqtt_publish(mqtts_dev->client, target, &msg, timeout_ms);
}

int32_t iot_mqtts_notify(void *dev, struct iot_data_t *data, uint32_t timeout_ms)
{
    struct iot_mqtts_t *mqtts_dev    = (struct iot_mqtts_t *)dev;
    uint8_t *           raw_data     = NULL;
    uint32_t            raw_data_len = 0;
    int32_t             ret          = ERR_OK;
    handle_t            cd_hdl       = countdown_start(timeout_ms);

    /** 根据iot_data_t构造payload数据*/
    if (NULL == (raw_data = osl_malloc(CONFIG_PACKET_PAYLOAD_LEN_MAX)))
    {
        ret = ERR_NO_MEM;
        goto exit;
    }
    osl_memset(raw_data, 0, CONFIG_PACKET_PAYLOAD_LEN_MAX);

    raw_data_len = construct_payload(mqtts_dev->msg_id++, raw_data, data);
    if (raw_data_len)
    {
        ret = iot_mqtts_push_rawdata(dev, mqtts_dev->topic.post, raw_data, raw_data_len, countdown_left(cd_hdl));
    }
    osl_free(raw_data);
    mqtts_dev->reply_flag = 0;
    do
    {
        ret = iot_mqtts_step(dev, countdown_left(cd_hdl));
        if (0 > ret)
        {
            goto exit;
        }
        else if (mqtts_dev->reply_flag)
        {
            if (mqtts_dev->reply_id == (mqtts_dev->msg_id - 1))
            {
                if (mqtts_dev->reply_accept)
                {
                    ret = ERR_OK;
                }
                else
                {
                    ret = ERR_INVALID_DATA;
                }
                break;
            }
        }
    } while (0 == countdown_is_expired(cd_hdl));

exit:
    countdown_stop(cd_hdl);
    return ret;
}

int32_t iot_mqtts_observe(void *dev, const uint8_t *resource, uint32_t timeout_ms)
{
    struct iot_mqtts_t *mqtts_dev = (struct iot_mqtts_t *)dev;

    return mqtt_subscribe(mqtts_dev->client, resource, MQTT_QOS1, iot_mqtts_message_arrived, dev, timeout_ms);
}

int32_t iot_mqtts_observe_cancel(void *dev, const uint8_t *resource, uint32_t timeout_ms)
{
    struct iot_mqtts_t *mqtts_dev = (struct iot_mqtts_t *)dev;

    return mqtt_unsubscribe(mqtts_dev->client, resource, timeout_ms);
}

int32_t iot_mqtts_close(void *dev, uint32_t timeout_ms)
{
    struct iot_mqtts_t *mqtts_dev = (struct iot_mqtts_t *)dev;

    mqtt_disconnect(mqtts_dev->client, timeout_ms);
    iot_mqtts_topic_free(mqtts_dev);
    iot_mqtts_deinit(mqtts_dev);

    return ERR_OK;
}
