/**
 * Copyright (c), 2012~2021 iot.10086.cn All Rights Reserved
 *
 * @file ringa_api.c
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "err_def.h"
#include "log.h"
#include "utils.h"
#include "plat_osl.h"
#include "plat_time.h"
#include "mqtt_api.h"

#include "ringa_tlv.h"
#include "ringa_cmd.h"
#include "ringa_api.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#if defined(CONFIG_RINGA_REGION_MAINLAND) // Production Env
#define SERVER_ADDR "183.230.40.32"
#elif defined(CONFIG_RINGA_REGION_HK) // Production Env for HongKong
#define SERVER_ADDR "47.244.72.233"
#elif defined(CONFIG_RINGA_REGION_GLB) // Production Env for Global Foreign
#define SERVER_ADDR "54.195.112.250"
#endif
#ifdef CONFIG_RINGA_TLS
#define SERVER_PORT 8883
#else
#define SERVER_PORT 1883
#endif

#ifdef CONFIG_RINGA_TLS
static const uint8_t *srv_ca_cert = {"-----BEGIN CERTIFICATE-----\r\n"
                                     "MIIDszCCApugAwIBAgIJAJzZf51/tb/VMA0GCSqGSIb3DQEBCwUAMHAxCzAJBgNV\r\n"
                                     "BAYTAkNOMRIwEAYDVQQIDAlDaG9uZ1FpbmcxEjAQBgNVBAcMCUNob25nUWluZzEO\r\n"
                                     "MAwGA1UECgwFQ01JT1QxDTALBgNVBAsMBFBBQVMxGjAYBgNVBAMMEW9wZW4uaW90\r\n"
                                     "LjEwMDg2LmNuMB4XDTE4MDcyNDA3NTU1NloXDTQ4MDcxNjA3NTU1NlowcDELMAkG\r\n"
                                     "A1UEBhMCQ04xEjAQBgNVBAgMCUNob25nUWluZzESMBAGA1UEBwwJQ2hvbmdRaW5n\r\n"
                                     "MQ4wDAYDVQQKDAVDTUlPVDENMAsGA1UECwwEUEFBUzEaMBgGA1UEAwwRb3Blbi5p\r\n"
                                     "b3QuMTAwODYuY24wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDtbc1X\r\n"
                                     "Eo2MPBRoQBRsmf1vMPUDDio4N94bbZfvTMrd6XFidOWOWbwFM4HEWdcLOtGCUmtB\r\n"
                                     "JvNrXVKanEqU9tnvv5F6BJnj5ioqCeRzFJ6AIaomoemFLK8XfYqX8iXSOF33D1wI\r\n"
                                     "0W5C7IBD+twzr2CSSh6/3z59ggmOdhsOkD7mGaNybEpH4j8pFXvg4gwxaxsRVGhm\r\n"
                                     "nQqM1a66aJOlTI550hpmyWZQmeVoMlz+QdfncJfD6uO2RoX3HW9ADYURklYlkX+N\r\n"
                                     "l5mc5RA7UnaSBy+mepcD4yCoM5OImlJsnR+diNq6ckBMgwkvKAUt4A5xln1UJTtY\r\n"
                                     "9aEB5cg3PvviId4ZAgMBAAGjUDBOMB0GA1UdDgQWBBSeJUJyU1c7knc/8cLDQDcx\r\n"
                                     "TE+ZezAfBgNVHSMEGDAWgBSeJUJyU1c7knc/8cLDQDcxTE+ZezAMBgNVHRMEBTAD\r\n"
                                     "AQH/MA0GCSqGSIb3DQEBCwUAA4IBAQBlrom+ZJJx111WvhdfvZcuqbcaUnHjfohp\r\n"
                                     "Jp7cNAwUhDSzGrkT+pmFP/ONdi77WI8QnuI9SQOkQhGeV1r09Juq3MgDGvJD0MCy\r\n"
                                     "FqusP7uQafm1IrfU2rslcIhMx1rTVwvGSqrsTN7OeMKomFNHWOdyQLN8GmIL8zv+\r\n"
                                     "5z+fQSu/ORPgt9StU/Fi8ng8ZlIPl1rs5g2GMZI59+F86kBIruW+aTeJV9BgoG69\r\n"
                                     "+ivP8UGekvyT82bMvH/6GYi53xH6pn0HNw7inoHUGgd5bISI+alZ5PtgFyrjKWPb\r\n"
                                     "zoJC5xLYgXFkxWtkLBGGaKlVIJpHmtXznGgfz5rXYCqAGt1T/Q3I\r\n"
                                     "-----END CERTIFICATE-----"};
#endif

#define DATA_PUB_TOPIC "$sys/%s/%s/sqdata"
#define CMD_SUB_TOPIC  "$sys/%s/%s/creq/+"
#define CMD_RESP_TOPIC "$sys/%s/%s/crsp/"

#define DATA_CLIENT_SEND_BUF_LEN       1024
#define DATA_CLIENT_RECV_BUF_LEN       1024
#define DATA_CLIENT_DATA_PUB_TOPIC_LEN 128
#define DATA_CLIENT_DATA_SUB_TOPIC_LEN 128
#define DATA_CLIENT_CMD_RESP_TOPIC_LEN 128
#define CMD_ID_LEN_MAX                 32

#define DATA_DESC_STR "{\"ds_id\":\"%s\"}"

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
struct ringa_client_t
{
    void *       client;
    void *       buffer;
    uint8_t *    send_buf;
    uint8_t *    recv_buf;
    uint8_t *    cmd_sub_topic;
    uint8_t *    data_pub_topic;
    uint8_t *    cmd_resp_topic;
    uint8_t *    cmd_resp_id;
    ringa_cmd_cb cmd_callback;
};

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static struct ringa_client_t ringa_obj;

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
static void ringa_message_arrived(void *arg, const uint8_t *topic, struct mqtt_message_t *message)
{
    if (0 == osl_strncmp(topic, ringa_obj.cmd_sub_topic, osl_strlen(ringa_obj.cmd_sub_topic) - 1))
    {
        const uint8_t *cmd_id = topic + osl_strlen(ringa_obj.cmd_sub_topic) - 1;

        ringa_obj.cmd_callback(cmd_id, message->payload, message->payload_len);
    }
}

void ringa_set_callback(ringa_cmd_cb callback)
{
    ringa_obj.cmd_callback = callback;
}

int32_t ringa_login(const uint8_t *product_id, const uint8_t *dev_name, const uint8_t *dev_token, uint16_t life_time,
                    uint32_t timeout_ms)
{
    struct mqtt_param_t mqtt_param         = {0};
    uint16_t            cmd_sub_topic_len  = 0;
    uint16_t            cmd_resp_topic_len = 0;
    uint16_t            data_pub_topic_len = 0;
    uint16_t            malloc_buf_len     = 0;
    int32_t             ret                = ERR_OK;
    handle_t            cd_hdl             = 0;

    if ((NULL == product_id) || (NULL == dev_name) || (NULL == dev_token))
    {
        return ERR_INVALID_PARAM;
    }

    mqtt_param.recv_buf_len = CONFIG_RECV_BUF_LEN_MAX;
    mqtt_param.send_buf_len = CONFIG_SEND_BUF_LEN_MAX;
    cmd_sub_topic_len = osl_strlen((const uint8_t *)CMD_SUB_TOPIC) + osl_strlen(product_id) + osl_strlen(dev_name);
    cmd_resp_topic_len =
        osl_strlen((const uint8_t *)CMD_RESP_TOPIC) + osl_strlen(product_id) + osl_strlen(dev_name) + CMD_ID_LEN_MAX;
    data_pub_topic_len = osl_strlen((const uint8_t *)DATA_PUB_TOPIC) + osl_strlen(product_id) + osl_strlen(dev_name);

    malloc_buf_len =
        mqtt_param.recv_buf_len + mqtt_param.send_buf_len + cmd_sub_topic_len + cmd_resp_topic_len + data_pub_topic_len;
    if (NULL == (ringa_obj.buffer = osl_malloc(malloc_buf_len)))
    {
        return ERR_NO_MEM;
    }
    osl_memset(ringa_obj.buffer, 0, malloc_buf_len);

    cd_hdl = countdown_start(timeout_ms);

    mqtt_param.client_id    = dev_name;
    mqtt_param.username     = product_id;
    mqtt_param.password     = dev_token;
    mqtt_param.life_time    = life_time;
    mqtt_param.connect_flag = MQTT_CONNECT_FLAG_CLEAN_SESSION | MQTT_CONNECT_FLAG_USERNAME | MQTT_CONNECT_FLAG_PASSWORD;

    mqtt_param.send_buf = (uint8_t *)ringa_obj.buffer;
    mqtt_param.recv_buf = mqtt_param.send_buf + mqtt_param.send_buf_len;

#ifdef CONFIG_RINGA_TLS
    if (NULL
        == (ringa_obj.client = mqtt_connect((const uint8_t *)SERVER_ADDR, SERVER_PORT, srv_ca_cert,
                                            osl_strlen(srv_ca_cert), &mqtt_param, timeout_ms)))
#else
    if (NULL
        == (ringa_obj.client =
                mqtt_connect((const uint8_t *)SERVER_ADDR, SERVER_PORT, NULL, 0, &mqtt_param, timeout_ms)))
#endif
    {
        ret = ERR_NETWORK;
        goto exit1;
    }

    ringa_obj.cmd_sub_topic = mqtt_param.recv_buf + mqtt_param.recv_buf_len;
    osl_sprintf(ringa_obj.cmd_sub_topic, (const uint8_t *)CMD_SUB_TOPIC, product_id, dev_name);

    if (0
        != mqtt_subscribe(ringa_obj.client, ringa_obj.cmd_sub_topic, MQTT_QOS0, ringa_message_arrived,
                          (void *)&ringa_obj, countdown_left(cd_hdl)))
    {
        ret = ERR_REQUEST_FAILED;
        goto exit2;
    }

    ringa_obj.cmd_resp_topic = ringa_obj.cmd_sub_topic + cmd_sub_topic_len;
    osl_sprintf(ringa_obj.cmd_resp_topic, (const uint8_t *)CMD_RESP_TOPIC, product_id, dev_name);
    ringa_obj.cmd_resp_id    = ringa_obj.cmd_resp_topic + osl_strlen(ringa_obj.cmd_resp_topic);
    ringa_obj.data_pub_topic = ringa_obj.cmd_resp_topic + cmd_resp_topic_len;
    osl_sprintf(ringa_obj.data_pub_topic, (const uint8_t *)DATA_PUB_TOPIC, product_id, dev_name);

    goto exit;

exit2:
    mqtt_disconnect(ringa_obj.client, countdown_left(cd_hdl));
exit1:
    osl_free(ringa_obj.buffer);
exit:
    countdown_stop(cd_hdl);
    return ret;
}

int32_t ringa_logout(uint32_t timeout_ms)
{
    mqtt_disconnect(ringa_obj.client, timeout_ms);
    osl_free(ringa_obj.buffer);

    return ERR_OK;
}

int32_t ringa_notify(uint8_t *raw_data, uint16_t raw_data_len, const uint8_t *ds_id, uint32_t timeout_ms)
{
    struct mqtt_message_t msg           = {0};
    uint8_t *             payload       = NULL;
    uint32_t              payload_len   = 0;
    uint8_t               ds_id_str[64] = {0};
    uint32_t              ds_id_str_len = 0;
    uint32_t              offset        = 0;
    int32_t               ret           = ERR_OK;

    if (NULL == ds_id)
    {
        osl_sprintf(ds_id_str, (const uint8_t *)DATA_DESC_STR, "data");
    }
    else
    {
        osl_sprintf(ds_id_str, (const uint8_t *)DATA_DESC_STR, ds_id);
    }
    ds_id_str_len = osl_strlen((const uint8_t *)ds_id_str);
    payload_len   = 1 + 2 + ds_id_str_len + 4 + raw_data_len;
    if (NULL == (payload = osl_malloc(payload_len)))
    {
        return ERR_NO_MEM;
    }
    *(payload + offset++) = 0x02;
    offset += set_16bit_be(payload + offset, ds_id_str_len);
    osl_memcpy(payload + offset, (const void *)ds_id_str, ds_id_str_len);
    offset += ds_id_str_len;
    offset += set_32bit_be(payload + offset, raw_data_len);
    osl_memcpy(payload + offset, raw_data, raw_data_len);

    msg.payload     = payload;
    msg.payload_len = payload_len;
    if (0 == timeout_ms)
    {
        msg.qos = MQTT_QOS0;
    }
    else
    {
        msg.qos = MQTT_QOS1;
    }
    ret = mqtt_publish(ringa_obj.client, ringa_obj.data_pub_topic, &msg, timeout_ms);
    osl_free(payload);

    return ((ERR_OK == ret) ? ret : ERR_NETWORK);
}

int32_t ringa_cmd_resp(const uint8_t *cmd_id, uint8_t *cmd_resp, uint16_t cmd_resp_len, uint32_t timeout_ms)
{
    struct mqtt_message_t msg = {0};
    int32_t               ret = ERR_OK;

    msg.payload     = cmd_resp;
    msg.payload_len = cmd_resp_len;
    msg.qos         = MQTT_QOS0;
    osl_strcpy(ringa_obj.cmd_resp_id, cmd_id);
    ret = mqtt_publish(ringa_obj.client, ringa_obj.cmd_resp_topic, &msg, timeout_ms);

    return ((ERR_OK == ret) ? ret : ERR_NETWORK);
}

int32_t ringa_step(uint32_t timeout_ms)
{
    int32_t ret = ERR_OK;

    ret = mqtt_yield(ringa_obj.client, timeout_ms);

    return ((ERR_OK == ret) ? ret : ERR_NETWORK);
}

int32_t ringa_notify_sleep(uint32_t timeout_ms)
{
    uint32_t cmd         = RINGA_CMD_DEVICE_SLEEP_STATUS;
    uint8_t  payload[8]  = {0};
    uint16_t payload_len = 0;

    payload_len = ringa_tlv_ser(payload, RINGA_TLV_INT, (uint8_t *)(&cmd), sizeof(cmd));

    return ringa_notify(payload, payload_len, NULL, timeout_ms);
}

int32_t ringa_notify_wakeup(uint32_t timeout_ms)
{
    uint32_t cmd         = RINGA_CMD_DEVICE_SLEEP_STATUS;
    uint8_t  payload[8]  = {0};
    uint16_t payload_len = 0;

    payload_len = ringa_tlv_ser(payload, RINGA_TLV_INT, (uint8_t *)(&cmd), sizeof(cmd));

    return ringa_notify(payload, payload_len, NULL, timeout_ms);
}
