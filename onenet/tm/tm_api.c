/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file tm_api.c
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "dev_token.h"
#include "plat_osl.h"
#include "plat_time.h"
#include "tm_data.h"
#include "tm_onejson.h"
#include "tm_api.h"
#include "plat_osl.h"
#include "log.h"
#include "err_def.h"

#include <stdio.h>
#include <string.h>

#if defined(CONFIG_TM_MQTT)
#include "tm_mqtt.h"
#elif defined(CONFIG_TM_COAP)
#include "tm_coap.h"
#endif

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define TM_TOPIC_PREFIX "$sys/%s/%s/thing"

#define TM_TOPIC_PROP_POST            "/property/post"
#define TM_TOPIC_PROP_SET             "/property/set"
#define TM_TOPIC_PROP_GET             "/property/get"
#define TM_TOPIC_EVENT_POST           "/event/post"
#define TM_TOPIC_DESIRED_PROPS_GET    "/property/desired/get"
#define TM_TOPIC_DESIRED_PROPS_DELETE "/property/desired/delete"
#define TM_TOPIC_SERVICE_INVOKE       "/service/%s/invoke"
#define TM_TOPIC_PACK_DATA_POST       "/pack/post"
#define TM_TOPIC_HISTORY_DATA_POST    "/history/post"

#define TM_TOPIC_PROP_POST_REPLY            "/property/post/reply"
#define TM_TOPIC_PROP_SET_REPLY             "/property/set_reply"
#define TM_TOPIC_PROP_GET_REPLY             "/property/get_reply"
#define TM_TOPIC_EVENT_POST_REPLY           "/event/post/reply"
#define TM_TOPIC_DESIRED_PROPS_GET_REPLY    "/property/desired/get/reply"
#define TM_TOPIC_DESIRED_PROPS_DELETE_REPLY "/property/desired/delete/reply"
#define TM_TOPIC_SERVICE_INVOKE_REPLY       "/service/%s/invoke_reply"
#define TM_TOPIC_PACK_DATA_POST_REPLY       "/pack/post/reply"
#define TM_TOPIC_HISTORY_DATA_POST_REPLY    "/history/post/reply"

#ifdef CONFIG_GENERAL_OTA
#define TM_TOPIC_OTA_PREFIX "$sys/%s/%s/ota"
#define TM_TOPIC_OTA_INFORM "/inform"
#define TM_TOPIC_OTA_REPLY  "/inform_reply"
#endif
/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
#define REPLY_STATUS_NONE     0
#define REPLY_STATUS_WAIT     1
#define REPLY_STATUS_RECEIVED 2
struct tm_reply_info_t
{
    uint8_t  reply_id[16];
    int32_t  reply_code;
    void *   reply_data;
    uint8_t  resp_flag;
    uint16_t reply_status;
    uint8_t  reply_as_raw;
};

struct tm_obj_t
{
    struct tm_downlink_tbl_t downlink_tbl;
    uint8_t *                topic_prefix;
#ifdef CONFIG_GENERAL_OTA
    uint8_t *ota_topic_prefix;
#endif
    int32_t                post_id;
    struct tm_reply_info_t reply_info;
#ifdef CONFIG_TM_GATEWAY
    tm_subdev_cb subdev_callback;
#endif
#ifdef CONFIG_TM_FMS
    uint8_t *product_id;
    uint8_t *dev_name;
    uint8_t *dev_token;
#endif
};

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
struct tm_obj_t g_tm_obj;

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
static uint8_t *construct_topic(const uint8_t *prefix, const uint8_t *suffix)
{
    uint32_t topic_len = osl_strlen(prefix) + osl_strlen(suffix) + 1;
    uint8_t *topic_buf = osl_malloc(topic_len);

    if (topic_buf)
    {
        osl_memset(topic_buf, 0, topic_len);
        osl_strcat(topic_buf, prefix);
        osl_strcat(topic_buf, suffix);
    }

    return topic_buf;
}

static int32_t get_post_id(void)
{
    if (0x7FFFFFFF == ++(g_tm_obj.post_id))
    {
        g_tm_obj.post_id = 1;
    }

    return g_tm_obj.post_id;
}

int32_t tm_send_response(const uint8_t *name, uint8_t *msg_id, int32_t msg_code, uint8_t as_raw, void *resp_data,
                         uint32_t resp_data_len, uint32_t timeout_ms)
{
#if defined(CONFIG_TM_MQTT)
    uint8_t *topic = NULL;
#endif
    uint8_t *payload     = NULL;
    uint32_t payload_len = 0;

    if (NULL == (payload = osl_malloc(CONFIG_PACKET_PAYLOAD_LEN_MAX)))
    {
        return ERR_NO_MEM;
    }

    osl_memset(payload, 0, CONFIG_PACKET_PAYLOAD_LEN_MAX);
    payload_len = tm_onejson_pack_reply(payload, msg_id, msg_code, resp_data, as_raw);
#if defined(CONFIG_TM_MQTT)
    topic = construct_topic(g_tm_obj.topic_prefix, name);
    tm_mqtt_send_packet(topic, payload, payload_len, timeout_ms);
    osl_free(topic);
#elif defined(CONFIG_TM_COAP)
    tm_coap_send_packet(NULL, payload, payload_len, timeout_ms);
#endif

    osl_free(payload);

    return ERR_OK;
}

#if defined(CONFIG_TM_MQTT)
static int32_t wait_post_reply(int32_t post_id, handle_t cd_hdl)
{
    uint8_t temp_id[16] = {0};
    int32_t ret         = ERR_TIMEOUT;

    osl_sprintf(temp_id, (const uint8_t *)"%d", post_id);

    do
    {
        ret = tm_mqtt_step(countdown_left(cd_hdl));

        if (0 > ret)
        {
            log_error("wait reply error\n");
            break;
        }
        else
        {
            if (REPLY_STATUS_RECEIVED == g_tm_obj.reply_info.reply_status)
            {
                if (0 == osl_strcmp(temp_id, g_tm_obj.reply_info.reply_id))
                {
                    g_tm_obj.reply_info.reply_status = REPLY_STATUS_NONE;
                    ret                              = ERR_OK;
                    break;
                }
                else
                {
                    g_tm_obj.reply_info.reply_status = REPLY_STATUS_WAIT;
                }
            }
        }
    } while (0 == countdown_is_expired(cd_hdl));

    return ret;
}
#endif

int32_t tm_send_request(const uint8_t *name, uint8_t as_raw, void *data, uint32_t data_len, void **reply_data,
                        uint32_t *reply_data_len, uint32_t timeout_ms)
{
    uint8_t *topic       = NULL;
    uint8_t *payload     = NULL;
    uint32_t payload_len = 0;
    int32_t  post_id     = get_post_id();
    handle_t cd_hdl      = 0;
    int32_t  ret         = ERR_OTHERS;

    cd_hdl = countdown_start(timeout_ms);

    if (NULL == (payload = osl_malloc(CONFIG_PACKET_PAYLOAD_LEN_MAX)))
    {
        return ERR_NO_MEM;
    }

    osl_memset(payload, 0, CONFIG_PACKET_PAYLOAD_LEN_MAX);
    payload_len                      = tm_onejson_pack_request(payload, post_id, data, as_raw);
    g_tm_obj.reply_info.reply_as_raw = as_raw;

    topic = construct_topic(g_tm_obj.topic_prefix, name);
#if defined(CONFIG_TM_MQTT)
    ret = tm_mqtt_send_packet(topic, payload, payload_len, countdown_left(cd_hdl));
    if (ERR_OK == ret)
    {
        g_tm_obj.reply_info.reply_status = REPLY_STATUS_WAIT;
        if (0 == wait_post_reply(post_id, cd_hdl))
        {
            log_debug("reply err code: %d\n", g_tm_obj.reply_info.reply_code);
            if (200 == g_tm_obj.reply_info.reply_code)
            {
                log_debug("post data ok\n");
                if (NULL != reply_data)
                {
                    *reply_data = g_tm_obj.reply_info.reply_data;
                }
                ret = ERR_OK;
            }
            else
            {
                ret = ERR_OTHERS;
            }
            g_tm_obj.reply_info.reply_data = NULL;
        }
        g_tm_obj.reply_info.reply_code   = 0;
        g_tm_obj.reply_info.reply_as_raw = 0;
    }

#elif defined(CONFIG_TM_COAP)
    ret = tm_coap_send_packet(topic, payload, payload_len, countdown_left(cd_hdl));
    if (ERR_OK == ret)
    {
        log_debug("tm_send_request ok.\n");
    }
    else
    {
        log_debug("tm_send_request failed.\n");
    }
#endif

    osl_free(topic);
    osl_free(payload);
    countdown_stop(cd_hdl);

    return ret;
}

static int32_t tm_prop_set_handle(const uint8_t *name, void *res)
{
    uint16_t i   = 0;
    int32_t  ret = 0;

    for (i = 0; i < g_tm_obj.downlink_tbl.prop_tbl_size; i++)
    {
        if (0 == osl_strcmp(name, g_tm_obj.downlink_tbl.prop_tbl[i].name))
        {
            ret = g_tm_obj.downlink_tbl.prop_tbl[i].tm_prop_wr_cb(res);
            break;
        }
    }

    return ret;
}

static void tm_prop_set(uint8_t *payload, uint32_t payload_len)
{
    void *  props_data = NULL;
    uint8_t id[16]     = {0};

    props_data = tm_onejson_parse_request(payload, payload_len, id, 0);

    if (NULL != props_data)
    {
        if (0 == tm_data_list_each(props_data, tm_prop_set_handle))
        {
            tm_send_response((const uint8_t *)TM_TOPIC_PROP_SET_REPLY, id, 200, 0, NULL, 0,
                             CONFIG_DEFAULT_REQUEST_TIMEOUT);
        }
        else
        {
            tm_send_response((const uint8_t *)TM_TOPIC_PROP_SET_REPLY, id, 100, 0, NULL, 0,
                             CONFIG_DEFAULT_REQUEST_TIMEOUT);
        }

        tm_data_delete(props_data);
    }
}

static void tm_prop_get(uint8_t *payload, uint32_t payload_len)
{
    void *   props_data    = NULL;
    uint8_t *reply_payload = NULL;
    uint8_t  id[16]        = {0};

    if (NULL == (reply_payload = osl_malloc(128)))
    {
        log_error("prop set error, malloc failed.\n");
        return;
    }
    osl_memset(reply_payload, 0, 128);

    props_data = tm_onejson_parse_request(payload, payload_len, id, 0);

    if (NULL != props_data)
    {
        uint32_t i, j = 0;
        uint32_t prop_cnt   = tm_data_array_size(props_data);
        uint8_t *prop_name  = NULL;
        void *   reply_data = tm_data_create();

        for (i = 0; i < prop_cnt; i++)
        {
            tm_data_get_string(tm_data_array_get_element(props_data, i), &prop_name);
            for (j = 0; j < g_tm_obj.downlink_tbl.prop_tbl_size; j++)
            {
                if (0 == osl_strcmp(prop_name, g_tm_obj.downlink_tbl.prop_tbl[j].name))
                {
                    g_tm_obj.downlink_tbl.prop_tbl[j].tm_prop_rd_cb(reply_data);
                }
            }
        }

        tm_send_response((const uint8_t *)TM_TOPIC_PROP_GET_REPLY, id, 200, 0, reply_data, 0,
                         CONFIG_DEFAULT_REQUEST_TIMEOUT);

        tm_data_delete(props_data);
    }

    osl_free(reply_payload);
}

static void tm_post_reply(uint8_t *payload, uint32_t payload_len)
{
    if (REPLY_STATUS_WAIT == g_tm_obj.reply_info.reply_status)
    {
        g_tm_obj.reply_info.reply_data =
            tm_onejson_parse_reply(payload, payload_len, g_tm_obj.reply_info.reply_id, &g_tm_obj.reply_info.reply_code,
                                   g_tm_obj.reply_info.reply_as_raw);
        g_tm_obj.reply_info.reply_status = REPLY_STATUS_RECEIVED;
    }
}

#if 0
static int32_t tm_prop_set_desired_handle(const uint8_t *name, void *res)
{
    uint16_t i   = 0;
    int32_t  ret = 0;

    for (i = 0; i < g_tm_obj.downlink_tbl.prop_tbl_size; i++)
    {
        if (0 == osl_strcmp(name, g_tm_obj.downlink_tbl.prop_tbl[i].name))
        {
            ret = g_tm_obj.downlink_tbl.prop_tbl[i].tm_prop_wr_cb(res);
            break;
        }
    }

    return ret;
}
#endif

static void tm_service_invoke(uint8_t *svc_id, uint8_t *payload, uint32_t payload_len)
{
    void *   svc_data = NULL;
    uint8_t *topic    = NULL;
    uint32_t i        = 0;
    uint8_t  id[16]   = {0};

    svc_data = tm_onejson_parse_request(payload, payload_len, id, 0);

    if (NULL != svc_data)
    {
        void *reply_data = tm_data_struct_create();

        for (i = 0; i < g_tm_obj.downlink_tbl.svc_tbl_size; i++)
        {
            if (0 == osl_strcmp(svc_id, g_tm_obj.downlink_tbl.svc_tbl[i].name))
            {
                g_tm_obj.downlink_tbl.svc_tbl[i].tm_svc_cb(svc_data, reply_data);

                tm_data_delete(svc_data);
#if defined(CONFIG_TM_MQTT)
                topic = osl_malloc(osl_strlen((const uint8_t *)TM_TOPIC_SERVICE_INVOKE_REPLY) + osl_strlen(svc_id));
                osl_sprintf(topic, (const uint8_t *)TM_TOPIC_SERVICE_INVOKE_REPLY, svc_id);
#endif
                tm_send_response(topic, id, 200, 0, reply_data, 0, CONFIG_DEFAULT_REQUEST_TIMEOUT);
#if defined(CONFIG_TM_MQTT)
                osl_free(topic);
#endif
            }
        }
    }
}

#ifdef CONFIG_GENERAL_OTA
static void tm_ota(uint8_t *payload, uint32_t payload_len)
{
    void *   ota_data             = NULL;
    uint8_t  id[16]               = {0};
    // for response
    int8_t * topic                = NULL;
    uint8_t *response_payload     = NULL;
    uint32_t response_payload_len = 0;

    ota_data = tm_onejson_parse_request(payload, payload_len, id, 1);

    if (NULL != ota_data)
    {
        osl_free(ota_data);
        if (NULL == (response_payload = osl_malloc(CONFIG_PACKET_PAYLOAD_LEN_MAX)))
        {
            return;
        }

        osl_memset(response_payload, 0, CONFIG_PACKET_PAYLOAD_LEN_MAX);
        if (0 == g_tm_obj.downlink_tbl.ota_cb())
        {
            response_payload_len = tm_onejson_pack_reply(response_payload, id, 200, NULL, 0);
        }
        else
        {
            response_payload_len = tm_onejson_pack_reply(response_payload, id, 100, NULL, 0);
        }

#if defined(CONFIG_TM_MQTT)
        topic = construct_topic(g_tm_obj.ota_topic_prefix, TM_TOPIC_OTA_REPLY);
        tm_mqtt_send_packet(topic, response_payload, response_payload_len, CONFIG_DEFAULT_REQUEST_TIMEOUT);
        osl_free(topic);
#elif defined(CONFIG_TM_COAP)
        tm_coap_send_packet(NULL, response_payload, response_payload_len, CONFIG_DEFAULT_REQUEST_TIMEOUT);
#endif
        osl_free(response_payload);
        return;
    }
    return;
}
#endif

static int32_t check_action_by_topic(const uint8_t *action, const uint8_t *topic)
{
    return osl_strncmp(action, topic, osl_strlen(topic));
}

static int32_t tm_data_parse(const uint8_t *res_name, uint8_t *payload, uint32_t payload_len)
{
    const uint8_t *action = res_name + osl_strlen(g_tm_obj.topic_prefix);

    if (0 == check_action_by_topic(action, (const uint8_t *)TM_TOPIC_PROP_SET))
    {
        tm_prop_set(payload, payload_len);
    }
    else if (0 == check_action_by_topic(action, (const uint8_t *)TM_TOPIC_PROP_GET))
    {
        tm_prop_get(payload, payload_len);
    }
    else if ((0 == check_action_by_topic(action, (const uint8_t *)TM_TOPIC_PROP_POST))
             || (0 == check_action_by_topic(action, (const uint8_t *)TM_TOPIC_EVENT_POST))
             || (0 == check_action_by_topic(action, (const uint8_t *)TM_TOPIC_DESIRED_PROPS_GET))
             || (0 == check_action_by_topic(action, (const uint8_t *)TM_TOPIC_DESIRED_PROPS_DELETE))
             || (0 == check_action_by_topic(action, (const uint8_t *)TM_TOPIC_PACK_DATA_POST))
             || (0 == check_action_by_topic(action, (const uint8_t *)TM_TOPIC_HISTORY_DATA_POST)))
    {
        tm_post_reply(payload, payload_len);
    }
    else if (0 == check_action_by_topic(action, (const uint8_t *)"/service/"))
    {
        uint8_t        svc_id[32] = {0};
        uint8_t *      tmp_ptr    = NULL;
        const uint8_t *svc_id_ptr = res_name + osl_strlen(g_tm_obj.topic_prefix) + 9;

        tmp_ptr = osl_strstr(svc_id_ptr, (const uint8_t *)"/");
        osl_memcpy(svc_id, svc_id_ptr, tmp_ptr - svc_id_ptr);
        log_debug("Service Invoke [%s]\n", svc_id);

        tm_service_invoke(svc_id, payload, payload_len);
    }
#ifdef CONFIG_TM_GATEWAY
    else if (0 == check_action_by_topic(action, (const uint8_t *)"/sub/"))
    {
        if (g_tm_obj.subdev_callback)
        {
            g_tm_obj.subdev_callback(action, payload, payload_len);
        }
        if (NULL != osl_strstr(action, (const uint8_t *)"/reply/"))
        {
            tm_post_reply(payload, payload_len);
        }
    }
#endif
#ifdef CONFIG_GENERAL_OTA
    else
    {
        const uint8_t *ota_action = res_name + osl_strlen(g_tm_obj.ota_topic_prefix);

        if (0 == check_action_by_topic(ota_action, (const uint8_t *)TM_TOPIC_OTA_INFORM))
        {
            tm_ota(payload, payload_len);
        }
    }
#endif
    return 0;
}

int32_t tm_init(struct tm_downlink_tbl_t *downlink_tbl)
{
    osl_memset(&g_tm_obj, 0, sizeof(g_tm_obj));
    g_tm_obj.downlink_tbl.prop_tbl      = downlink_tbl->prop_tbl;
    g_tm_obj.downlink_tbl.prop_tbl_size = downlink_tbl->prop_tbl_size;
    g_tm_obj.downlink_tbl.svc_tbl       = downlink_tbl->svc_tbl;
    g_tm_obj.downlink_tbl.svc_tbl_size  = downlink_tbl->svc_tbl_size;
#ifdef CONFIG_GENERAL_OTA
    g_tm_obj.downlink_tbl.ota_cb = downlink_tbl->ota_cb;
#endif

#if defined(CONFIG_TM_MQTT)
    return tm_mqtt_init(tm_data_parse);
#elif defined(CONFIG_TM_COAP)
    return tm_coap_init(tm_data_parse);
#endif
}

int32_t tm_deinit(void)
{
#if defined(CONFIG_TM_MQTT)
    return tm_mqtt_deinit();
#elif defined(CONFIG_TM_COAP)
    return tm_coap_deinit();
#endif
}

#ifdef CONFIG_TM_GATEWAY
int32_t tm_set_subdev_callback(tm_subdev_cb callback)
{
    g_tm_obj.subdev_callback = callback;

    return ERR_OK;
}
#endif

int32_t tm_login(const uint8_t *product_id, const uint8_t *dev_name, const uint8_t *access_key, uint32_t timeout_ms)
{
    int32_t ret            = ERR_OK;
    uint8_t dev_token[256] = {0};

    dev_token_generate(dev_token, SIG_METHOD_SHA1, 2524579200, product_id, dev_name, access_key);
    log_debug("token = %s\n", dev_token);

#if defined(CONFIG_TM_MQTT)
    ret = tm_mqtt_login(product_id, dev_name, dev_token, CONFIG_ACCESS_LIFE_TIME, timeout_ms);
#elif defined(CONFIG_TM_COAP)
    ret = tm_coap_login(product_id, dev_name, dev_token, CONFIG_ACCESS_LIFE_TIME, timeout_ms);
#endif

    if (ERR_OK == ret)
    {
        uint32_t topic_malloc_len = 0;
        uint32_t topic_prefix_len =
            osl_strlen((const uint8_t *)TM_TOPIC_PREFIX) + osl_strlen(product_id) + osl_strlen(dev_name);
#ifdef CONFIG_GENERAL_OTA
        uint32_t ota_topic_prefix_len =
            osl_strlen((const uint8_t *)TM_TOPIC_OTA_PREFIX) + osl_strlen(product_id) + osl_strlen(dev_name);

        topic_malloc_len = topic_prefix_len + ota_topic_prefix_len;
#else
        topic_malloc_len = topic_prefix_len;
#endif
        g_tm_obj.topic_prefix = osl_malloc(topic_malloc_len);
        if (NULL == g_tm_obj.topic_prefix)
        {
#if defined(CONFIG_TM_MQTT)
            tm_mqtt_logout(timeout_ms);
#elif defined(CONFIG_TM_COAP)
            tm_coap_logout(timeout_ms);
#endif
            return ERR_NO_MEM;
        }
        osl_memset(g_tm_obj.topic_prefix, 0, topic_malloc_len);
        osl_sprintf(g_tm_obj.topic_prefix, (const uint8_t *)TM_TOPIC_PREFIX, product_id, dev_name);
#ifdef CONFIG_GENERAL_OTA
        g_tm_obj.ota_topic_prefix = g_tm_obj.topic_prefix + topic_prefix_len;
        osl_sprintf(g_tm_obj.ota_topic_prefix, (const uint8_t *)TM_TOPIC_OTA_PREFIX, product_id, dev_name);
#endif

#ifdef CONFIG_TM_FMS
        g_tm_obj.product_id = osl_strdup(product_id);
        g_tm_obj.dev_name   = osl_strdup(dev_name);
        g_tm_obj.dev_token  = osl_strdup(dev_token);
#endif
    }

    return ret;
}

int32_t tm_logout(uint32_t timeout_ms)
{
#ifdef CONFIG_TM_FMS
    osl_free(g_tm_obj.product_id);
    osl_free(g_tm_obj.dev_name);
    osl_free(g_tm_obj.dev_token);
#endif

    if (g_tm_obj.topic_prefix)
    {
        osl_free(g_tm_obj.topic_prefix);
        g_tm_obj.topic_prefix = NULL;
    }
#if defined(CONFIG_TM_MQTT)
    return tm_mqtt_logout(timeout_ms);
#elif defined(CONFIG_TM_COAP)
    return tm_coap_logout(timeout_ms);
#endif
}

int32_t tm_post_raw(const uint8_t *name, uint8_t *raw_data, uint32_t raw_data_len, uint8_t **reply_data,
                    uint32_t *reply_data_len, uint32_t timeout_ms)
{
    return tm_send_request(name, 1, raw_data, raw_data_len, (void **)reply_data, reply_data_len, timeout_ms);
}

int32_t tm_post_property(void *prop_data, uint32_t timeout_ms)
{
    return tm_send_request((const uint8_t *)TM_TOPIC_PROP_POST, 0, prop_data, 0, NULL, NULL, timeout_ms);
}

int32_t tm_post_event(void *event_data, uint32_t timeout_ms)
{
    return tm_send_request((const uint8_t *)TM_TOPIC_EVENT_POST, 0, event_data, 0, NULL, NULL, timeout_ms);
}

int32_t tm_get_desired_props(uint32_t timeout_ms)
{
    void *   prop_list  = tm_data_array_create(g_tm_obj.downlink_tbl.prop_tbl_size);
    uint32_t i          = 0;
    int32_t  ret        = ERR_OTHERS;
    void *   reply_data = NULL;

    for (i = 0; i < g_tm_obj.downlink_tbl.prop_tbl_size; i++)
    {
        tm_data_array_set_string(prop_list, (uint8_t *)(g_tm_obj.downlink_tbl.prop_tbl[i].name));
    }

    ret = tm_send_request((const uint8_t *)TM_TOPIC_DESIRED_PROPS_GET, 0, prop_list, 0, &reply_data, NULL, timeout_ms);

    if (ERR_OK == ret)
    {
        tm_data_list_each(reply_data, tm_prop_set_handle);
        log_debug("get desired props ok\n");
    }
    if (NULL != reply_data)
    {
        tm_data_delete(reply_data);
    }

    return ret;
}

int32_t tm_delete_desired_props(uint32_t timeout_ms)
{
    void *   prop_list = tm_data_create();
    uint32_t i         = 0;
    int32_t  ret       = ERR_OTHERS;

    for (i = 0; i < g_tm_obj.downlink_tbl.prop_tbl_size; i++)
    {
        tm_data_struct_set_data(prop_list, (uint8_t *)(g_tm_obj.downlink_tbl.prop_tbl[i].name), NULL);
    }

    ret = tm_send_request((const uint8_t *)TM_TOPIC_DESIRED_PROPS_DELETE, 0, prop_list, 0, NULL, NULL, timeout_ms);

    return ret;
}

void *tm_pack_device_data(void *data, const uint8_t *product_id, const uint8_t *dev_name, void *prop, void *event,
                          int8_t as_raw)
{
    return tm_onejson_pack_props_and_events(data, product_id, dev_name, prop, event, as_raw);
}

int32_t tm_post_pack_data(void *pack_data, uint32_t timeout_ms)
{
    return tm_send_request((const uint8_t *)TM_TOPIC_PACK_DATA_POST, 0, pack_data, 0, NULL, 0, timeout_ms);
}

int32_t tm_post_history_data(void *history_data, uint32_t timeout_ms)
{
    return tm_send_request((const uint8_t *)TM_TOPIC_HISTORY_DATA_POST, 0, history_data, 0, NULL, 0, timeout_ms);
}

int32_t tm_step(uint32_t timeout_ms)
{
#if defined(CONFIG_TM_MQTT)
    return tm_mqtt_step(timeout_ms);
#elif defined(CONFIG_TM_COAP)
    return tm_coap_step(timeout_ms);
#endif
}

#ifdef CONFIG_TM_FMS
#include "wolfssl/wolfcrypt/hash.h"
#include "http_api.h"
#include "utils.h"

int32_t tm_file_get(tm_file_cb callback, uint8_t *file_id, uint32_t file_size, uint32_t segment_size,
                    uint32_t timeout_ms)
{
    handle_t              cd_hdl                = countdown_start(timeout_ms);
    void *                http_ctx              = NULL;
    uint8_t               http_path[128]        = {0};
    uint8_t               http_header_range[32] = {0};
    uint32_t              range_start           = 0;
    uint32_t              range_end             = 0;
    struct tm_file_data_t file_data             = {0};
    int32_t               ret                   = 0;

    if ((NULL == callback) || (NULL == file_id) || (0 == file_size))
    {
        countdown_stop(timeout_ms);
        return ERR_INVALID_PARAM;
    }

    osl_sprintf(http_path, (const uint8_t *)"/studio/%s/%s/%s/outdownload", g_tm_obj.product_id, g_tm_obj.dev_name,
                file_id);
    do
    {
        range_start = (range_end) ? (range_end + 1) : 0;
        range_end   = ((file_size - range_start) > segment_size) ? (range_start + segment_size - 1) : 0;

        http_ctx =
            http_new(HTTP_METHOD_GET, (const uint8_t *)"studio-file.heclouds.com", http_path, segment_size + 800);
        http_add_header(http_ctx, (const uint8_t *)"Content-Type", (const uint8_t *)"application/json");
        http_add_header(http_ctx, (const uint8_t *)"Authorization", g_tm_obj.dev_token);

        osl_sprintf(http_header_range, (const uint8_t *)"bytes=%d-", range_start);
        if (range_end)
        {
            osl_sprintf(http_header_range + osl_strlen(http_header_range), (const uint8_t *)"%d", range_end);
        }
        http_add_header(http_ctx, (const uint8_t *)"Range", http_header_range);

        ret = http_send_request(http_ctx, (const uint8_t *)"183.230.102.116", 27001, &(file_data.data),
                                &(file_data.data_len), timeout_ms);
        if ((200 <= ret) && (300 > ret))
        {
            file_data.seq = range_start;
            callback(TM_FILE_EVENT_GET_DATA, (void *)&file_data);
        }
        else
        {
            http_delete(http_ctx);
            countdown_stop(cd_hdl);
            return ret;
        }
        http_delete(http_ctx);
    } while ((0 == countdown_is_expired(cd_hdl)) && (0 != range_end));

    countdown_stop(cd_hdl);

    return ERR_OK;
}

static void fp_add_form_data(void *http_ctx, uint8_t *boundary, uint8_t *name, uint8_t *value, uint32_t value_len,
                             const uint8_t *addl)
{
    http_add_body(http_ctx, (uint8_t *)"--", 2);
    http_add_body(http_ctx, boundary, osl_strlen(boundary));
    if (name)
    {
        uint8_t str_tmp[128] = {0};

        http_add_body(http_ctx, (uint8_t *)"\r\n", 2);
        osl_sprintf(str_tmp, (const uint8_t *)"Content-Disposition:form-data;name=\"%s\"", name);
        if (addl)
        {
            osl_strcat(str_tmp, (const uint8_t *)";");
            osl_strcat(str_tmp, addl);
        }
        osl_strcat(str_tmp, (const uint8_t *)"\r\n\r\n");
        http_add_body(http_ctx, str_tmp, osl_strlen(str_tmp));
        http_add_body(http_ctx, value, value_len);
    }
    else
    {
        http_add_body(http_ctx, (uint8_t *)"--", 2);
    }
    http_add_body(http_ctx, (uint8_t *)"\r\n", 2);
}

int32_t tm_file_post(tm_file_cb callback, const uint8_t *product_id, const uint8_t *dev_name, const uint8_t *access_key,
                     const uint8_t *file_name, uint8_t *file, uint32_t file_size, uint32_t timeout_ms)
{
    uint8_t  boundary[]    = "----7MA4YWxkTrZu0gW";
    void *   http_ctx      = NULL;
    uint8_t *http_resp     = NULL;
    uint32_t http_resp_len = 0;
    uint8_t  file_md5[16]  = {0};
    uint8_t  str_tmp[256]  = {0};
    int32_t  ret           = ERR_OK;

    if ((NULL == callback) || (NULL == product_id) || (NULL == dev_name) || (NULL == access_key) || (NULL == file_name)
        || (NULL == file) || (0 == file_size))
    {
        return ERR_INVALID_PARAM;
    }

    osl_sprintf(str_tmp, (const uint8_t *)"/studio/%s/%s/outupload", product_id, dev_name);
    http_ctx = http_new(HTTP_METHOD_POST, (const uint8_t *)"studio-file.heclouds.com", str_tmp, file_size + 800);
    osl_sprintf(str_tmp, (const uint8_t *)"multipart/form-data;boundary=%s", boundary);
    http_add_header(http_ctx, (const uint8_t *)"Content-Type", str_tmp);
    dev_token_generate(str_tmp, SIG_METHOD_SHA1, 2524579200, product_id, dev_name, access_key);
    http_add_header(http_ctx, (const uint8_t *)"Authorization", str_tmp);

    /** Set Body*/
    wc_Md5Hash(file, file_size, file_md5);
    hex_to_str(str_tmp, file_md5, 16);
    str_tmp[32] = '\0';
    fp_add_form_data(http_ctx, boundary, (uint8_t *)"md5", str_tmp, osl_strlen(str_tmp), NULL);

    fp_add_form_data(http_ctx, boundary, (uint8_t *)"filename", (uint8_t *)file_name, osl_strlen(file_name), NULL);

    osl_sprintf(str_tmp, (const uint8_t *)"%d", file_size);
    fp_add_form_data(http_ctx, boundary, (uint8_t *)"size", str_tmp, osl_strlen(str_tmp), NULL);

    osl_sprintf(str_tmp, (const uint8_t *)"filename=\"%s\"", file_name);
    fp_add_form_data(http_ctx, boundary, (uint8_t *)"file", file, file_size, str_tmp);

    fp_add_form_data(http_ctx, boundary, NULL, NULL, 0, NULL);

    ret =
        http_send_request(http_ctx, (const uint8_t *)"183.230.102.116", 27001, &http_resp, &http_resp_len, timeout_ms);
    if (200 == ret)
    {
        if (http_resp)
        {
            uint8_t *tmp_ptr = osl_strstr(http_resp, (const uint8_t *)"\"uuid\":");
            uint8_t *cb_data = tmp_ptr + 8;

            if (tmp_ptr)
            {
                tmp_ptr  = osl_strstr(cb_data, (const uint8_t *)"\"");
                *tmp_ptr = '\0';
                callback(TM_FILE_EVENT_POST_SUCCESSED, cb_data);
                ret = ERR_OK;
            }
            else
            {
                tmp_ptr  = osl_strstr(http_resp, (const uint8_t *)"\"error\":");
                cb_data  = tmp_ptr + 9;
                tmp_ptr  = osl_strstr(cb_data, (const uint8_t *)"\"");
                *tmp_ptr = '\0';
                callback(TM_FILE_EVENT_POST_FAILED, cb_data);
                ret = ERR_OTHERS;
            }
        }
    }
    else
    {
        ret = ERR_OTHERS;
    }

    http_delete(http_ctx);

    return ret;
}
#endif
