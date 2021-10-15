/**
 * Copyright (c), 2012~2021 iot.10086.cn All Rights Reserved
 *
 * @file ringa_reg.c
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "err_def.h"
#include "plat_osl.h"
#include "plat_time.h"
#include "mqtt_api.h"
#include "log.h"

#include "ringa_tlv.h"
#include "ringa_cmd.h"
#include "ringa_api.h"
#include "ringa_reg.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#if defined(CONFIG_RINGA_REGION_MAINLAND) // Production Env
#define REGISTER_ADDR "183.230.40.32"
#elif defined(CONFIG_RINGA_REGION_HK) // Production Env for HongKong
#define REGISTER_ADDR "47.75.229.154"
#elif defined(CONFIG_RINGA_REGION_GLB) // Production Env for Global Foreign
#define REGISTER_ADDR "54.217.123.221"
#endif
#ifdef CONFIG_RINGA_TLS
#define REGISTER_PORT 26004
static const uint8_t *reg_ca_cert = {"-----BEGIN CERTIFICATE-----\r\n"
                                     "MIIDRDCCAiwCCQCLob+Kt+raNjANBgkqhkiG9w0BAQUFADBjMQswCQYDVQQGEwJD\r\n"
                                     "TjESMBAGA1UECBMJQ2hvbmdRaW5nMRIwEAYDVQQHEwlDaG9uZ1FpbmcxDjAMBgNV\r\n"
                                     "BAoTBUNNSU9UMQ0wCwYDVQQLEwRIRVdVMQ0wCwYDVQQDEwRyb290MCAXDTE4MDgw\r\n"
                                     "ODA3MTczOVoYDzIwNTEwNjE2MDcxNzM5WjBjMQswCQYDVQQGEwJDTjESMBAGA1UE\r\n"
                                     "CBMJQ2hvbmdRaW5nMRIwEAYDVQQHEwlDaG9uZ1FpbmcxDjAMBgNVBAoTBUNNSU9U\r\n"
                                     "MQ0wCwYDVQQLEwRIRVdVMQ0wCwYDVQQDEwRyb290MIIBIjANBgkqhkiG9w0BAQEF\r\n"
                                     "AAOCAQ8AMIIBCgKCAQEA0xFUS3vdSHJxM68ZSAct5FHn2/w9nfoAyQ3Fk/oS/yk5\r\n"
                                     "wfB6pBfZUNB/LZ3F38QNSeG21oo0mf7SEFFzJ08uxm5ukE7DWbyzG4U7cCCR2QOo\r\n"
                                     "rBDRffZ9FdxxLMTbOYsDUwV4/SNJVm5ElDEA1eBB2oGyQkpNRGiVnwIOpmI3q0vq\r\n"
                                     "w25FOebEraoW+zo7SkGD/7+Km4zKsirwkVTiMQ6H5KCQYG6EMnGel1DYKUHq/QWO\r\n"
                                     "RY96E5q9e4Wn7ceUt7ArrHn18Vem9ZowClXnde2Vy1Cz0FJkhBBOoHtQIVVkjJSt\r\n"
                                     "0ak241cEW98V+dvEFx81pIzM2REIgI3fneQeMe3PWQIDAQABMA0GCSqGSIb3DQEB\r\n"
                                     "BQUAA4IBAQAbtzMYNdhPLfsn1kTgTBSprj8v+T11OZysc2B1OU8GWgdHxCCICJLC\r\n"
                                     "j0K8zysCZaSNXo7Fl2WYCzJCoDkIEC9GTjISVbTCT17opbtF4SVoonSlPuOMmQeK\r\n"
                                     "ttc7k5NfQ4hNNyYHNDas8/+9sNGIrTnX1hSl1YuqpdsXC2eEQ81zaGOXUwFu32J1\r\n"
                                     "kH7pwGuzNLoLBJ+bRvKgb4Re36VCsoMlG32u9AidPx8dyMC7UudymtF0C6IAZToD\r\n"
                                     "DbFL+q5g4oFuP8vTLgZrKqera+WRA81uuurwHuiE3sedvCxhUeKO1Huow5H9UkX6\r\n"
                                     "KCmDgsh51FS/cWOzw5napuu/m9NUTPiV\r\n"
                                     "-----END CERTIFICATE-----"};
#else
#define REGISTER_PORT 26003
#endif

#define REG_KEEPALIVE_INTERVAL 60
#define REG_BUFFER_LEN_MAX     256
#define REG_PUB_TOPIC          "device/products/%s/devices/%s/command"
//#define REG_SUB_TOPIC        "device/products/%s/devices/%s/command_resp"

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static ringa_reg_evt_cb reg_evt_callback;
static uint8_t          reg_resp_got = 0;

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
static void reg_message_arrived(void *arg, const uint8_t *topic, struct mqtt_message_t *message)
{
    uint32_t cmd     = 0;
    uint8_t *cmd_ptr = 0;
    uint16_t cmd_len = 0;
    uint16_t offset  = 0;

    log_info("Got response from [%s]\n", topic);
    offset = ringa_tlv_deser(message->payload, NULL, &cmd_ptr, &cmd_len);
    osl_memcpy(&cmd, cmd_ptr, cmd_len);
    if (RINGA_CMD_DEVICE_TOKEN_RESP == cmd)
    {
        struct ringa_reg_result_t reg_result = {0};
        uint32_t                  buf_len    = 0;

        buf_len              = ringa_tlv_get_next_len(message->payload + offset) + 1;
        reg_result.dev_token = osl_malloc(buf_len);
        if (NULL != reg_result.dev_token)
        {
            osl_memset(reg_result.dev_token, 0, buf_len);
            offset += ringa_tlv_deser(message->payload + offset, NULL, &cmd_ptr, &cmd_len);
            osl_memcpy(reg_result.dev_token, cmd_ptr, cmd_len);

            buf_len           = ringa_tlv_get_next_len(message->payload + offset) + 1;
            reg_result.dev_id = osl_malloc(buf_len);
            if (NULL != reg_result.dev_id)
            {
                osl_memset(reg_result.dev_id, 0, buf_len);
                offset += ringa_tlv_deser(message->payload + offset, NULL, &cmd_ptr, &cmd_len);
                osl_memcpy(reg_result.dev_id, cmd_ptr, cmd_len);
                reg_evt_callback(RINGA_REG_EVT_SUCCESSED, &reg_result, sizeof(reg_result));
                osl_free(reg_result.dev_id);
            }
            osl_free(reg_result.dev_token);
        }
        else
        {
            reg_evt_callback(RINGA_REG_EVT_FAILED, NULL, 0);
        }
    }
    reg_resp_got = 1;
}

static int32_t pack_token_request(uint8_t *request, const uint8_t *connection_id)
{
    uint16_t offset   = 0;
    uint32_t cmd_type = RINGA_CMD_DEVICE_TOKEN_REQ;

    offset += ringa_tlv_ser(request + offset, RINGA_TLV_INT, (uint8_t *)&cmd_type, sizeof(cmd_type));
    offset += ringa_tlv_ser(request + offset, RINGA_TLV_STRING, (uint8_t *)connection_id, osl_strlen(connection_id));

    return offset;
}

int32_t ringa_register(const uint8_t *product_id, const uint8_t *dev_name, const uint8_t *connection_id,
                       const uint8_t *binding_token, ringa_reg_evt_cb reg_callback, uint32_t timeout_ms)
{
    void *                mqtt_client     = NULL;
    struct mqtt_param_t   mqtt_param      = {0};
    uint8_t *             subed_topic     = NULL;
    uint32_t              subed_topic_len = 0;
    uint8_t *             pub_topic       = NULL;
    struct mqtt_message_t req_token       = {0};
    uint8_t *             request_payload = {0};
    uint8_t *             malloc_buf      = NULL;
    uint32_t              malloc_buf_len  = 0;
    handle_t              cd_hdl          = 0;
    int32_t               ret             = ERR_OK;

    if ((NULL == product_id) || (NULL == dev_name) || (NULL == connection_id) || (NULL == binding_token)
        || (NULL == reg_callback))
    {
        return ERR_INVALID_PARAM;
    }

    mqtt_param.recv_buf_len = mqtt_param.send_buf_len = REG_BUFFER_LEN_MAX;
    subed_topic_len = osl_strlen((const uint8_t *)REG_PUB_TOPIC) + osl_strlen(product_id) + osl_strlen(dev_name) + 4;
    /** request payload len < 64*/
    malloc_buf_len  = 64 + mqtt_param.recv_buf_len + mqtt_param.send_buf_len + (subed_topic_len * 2);
    if (NULL == (malloc_buf = osl_malloc(malloc_buf_len)))
    {
        return ERR_NO_MEM;
    }
    osl_memset(malloc_buf, 0, malloc_buf_len);

    cd_hdl = countdown_start(timeout_ms);

    request_payload         = malloc_buf;
    mqtt_param.client_id    = dev_name;
    mqtt_param.username     = connection_id;
    mqtt_param.password     = binding_token;
    mqtt_param.life_time    = REG_KEEPALIVE_INTERVAL;
    mqtt_param.connect_flag = MQTT_CONNECT_FLAG_CLEAN_SESSION | MQTT_CONNECT_FLAG_USERNAME | MQTT_CONNECT_FLAG_PASSWORD;
    mqtt_param.send_buf     = request_payload + 64;
    mqtt_param.recv_buf     = mqtt_param.send_buf + mqtt_param.send_buf_len;

#ifdef CONFIG_RINGA_TLS
    if (NULL
        == (mqtt_client = mqtt_connect((const uint8_t *)REGISTER_ADDR, REGISTER_PORT, reg_ca_cert, osl_strlen(reg_ca_cert), &mqtt_param,
                                       countdown_left(cd_hdl))))
#else
    if (NULL
        == (mqtt_client = mqtt_connect((const uint8_t *)REGISTER_ADDR, REGISTER_PORT, NULL, 0, &mqtt_param, countdown_left(cd_hdl))))
#endif
    {
        ret = ERR_NETWORK;
        goto exit;
    }
    reg_evt_callback = reg_callback;
    reg_evt_callback(RINGA_REG_EVT_CONNECTED, NULL, 0);

    subed_topic = mqtt_param.recv_buf + mqtt_param.recv_buf_len;
    osl_sprintf(subed_topic, (const uint8_t *)REG_PUB_TOPIC "_resp", product_id, dev_name);
    if (0 != mqtt_subscribe(mqtt_client, subed_topic, MQTT_QOS0, reg_message_arrived, NULL, countdown_left(cd_hdl)))
    {
        ret = ERR_REQUEST_FAILED;
        goto exit1;
    }

    pub_topic = subed_topic + subed_topic_len;
    osl_sprintf(pub_topic, (const uint8_t *)REG_PUB_TOPIC, product_id, dev_name);
    req_token.payload_len = pack_token_request(request_payload, connection_id);
    req_token.payload     = request_payload;
    req_token.qos         = MQTT_QOS0;
    ret                   = mqtt_publish(mqtt_client, pub_topic, &req_token, countdown_left(cd_hdl));
    if (ERR_OK != ret)
    {
        ret = ERR_NETWORK;
        goto exit1;
    }

    reg_resp_got = 0;
    do
    {
        ret = mqtt_yield(mqtt_client, countdown_left(cd_hdl));
        if (0 > ret)
        {
            reg_evt_callback(RINGA_REG_EVT_FAILED, NULL, 0);
            break;
        }
        if (1 == countdown_is_expired(cd_hdl))
        {
            ret = ERR_TIMEOUT;
            break;
        }
    } while (0 == reg_resp_got);

exit1:
    mqtt_disconnect(mqtt_client, timeout_ms);
    reg_evt_callback(RINGA_REG_EVT_DISCONNECTED, NULL, 0);
exit:
    countdown_stop(cd_hdl);
    osl_free(malloc_buf);
    return ret;
}
