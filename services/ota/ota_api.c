/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file ota_api.c
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "plat_osl.h"
#include "http_api.h"
#include "ota_api.h"
#include "err_def.h"
#include "cJSON.h"
#include "log.h"

#ifdef CONFIG_TM
#include "dev_token.h"
#endif

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#ifdef CONFIG_TM
#define OTA_HOST_ADDR "studio-ota.heclouds.com"
#else
#define OTA_HOST_ADDR "ota.heclouds.com"
#endif
#define OTA_SERVER_ADDR "183.230.40.50"
#ifdef CONFIG_GENERAL_OTA_TLS
#define OTA_SERVER_PORT 443
#else
#define OTA_SERVER_PORT 80
#endif

#define HTTP_PACK_SIZE_LEN 512

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
struct ota_dev_info_t
{
    uint8_t *dev_token;
    uint8_t *version;
#ifndef CONFIG_TM
    uint8_t *dev_id;
    uint8_t *manuf;
    uint8_t *model;
    boolean  cdn;
#else
    uint8_t *product_id;
    uint8_t *dev_name;
#endif
    uint8_t type;
};

struct ota_contex_t
{
    struct ota_dev_info_t dev_info;
    ota_evt_cb            evt_cb;
};

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
struct ota_contex_t *ota_ctx = NULL;

#ifdef CONFIG_GENERAL_OTA_TLS
static const uint8_t *ota_ca_cert = {"-----BEGIN CERTIFICATE-----\r\n"
                                     "MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\r\n"
                                     "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"
                                     "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\r\n"
                                     "QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\r\n"
                                     "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n"
                                     "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\r\n"
                                     "9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\r\n"
                                     "CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\r\n"
                                     "nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\r\n"
                                     "43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\r\n"
                                     "T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\r\n"
                                     "gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\r\n"
                                     "BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\r\n"
                                     "TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\r\n"
                                     "DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\r\n"
                                     "hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\r\n"
                                     "06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\r\n"
                                     "PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\r\n"
                                     "YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\r\n"
                                     "CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\r\n"
                                     "-----END CERTIFICATE-----"};
#endif

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
static void ota_free(void *ptr)
{
    if (NULL != ptr)
    {
        osl_free(ptr);
        ptr = NULL;
    }
}

static int32_t ota_errno_parse(cJSON *root)
{
    cJSON *item = NULL;

    item = cJSON_GetObjectItem(root, "errno");
    if (NULL != item)
    {
        return item->valueint;
    }
    else
    {
        return ERR_NO_MEM;
    }
}

static int32_t ota_common_resp_parse(uint8_t *data, uint32_t data_len)
{
    cJSON * root = NULL;
    int32_t ret  = ERR_NO_MEM;

    if (NULL != (root = cJSON_ParseWithLength(data, data_len)))
    {
        ret = ota_errno_parse(root);
        cJSON_Delete(root);
    }
    return ret;
}

static int32_t ota_check_task_parse(struct ota_task_info_t *task_info, uint8_t *data, uint32_t data_len)
{
    cJSON * root      = NULL;
    cJSON * item_data = NULL;
    cJSON * item      = NULL;
    int32_t ret       = ERR_NO_MEM;

    if (NULL != (root = cJSON_ParseWithLength(data, data_len)))
    {
        if (0 == (ret = ota_errno_parse(root)))
        {
            if (NULL != (item_data = cJSON_GetObjectItem(root, "data")))
            {
                if (NULL != (item = cJSON_GetObjectItem(item_data, "target")))
                    osl_strcpy(task_info->target, item->valuestring);
                if (NULL != (item = cJSON_GetObjectItem(item_data, "md5")))
                    osl_strcpy(task_info->md5, item->valuestring);
                if (NULL != (item = cJSON_GetObjectItem(item_data, "type")))
                    task_info->type = item->valueint;
                if (NULL != (item = cJSON_GetObjectItem(item_data, "size")))
                    task_info->size = item->valueint;
#ifndef CONFIG_TM
                if (NULL != (item = cJSON_GetObjectItem(item_data, "token")))
                    osl_strcpy(task_info->token, item->valuestring);
                if (NULL != (item = cJSON_GetObjectItem(item_data, "ipPort")))
                    osl_strcpy(task_info->ip, item->valuestring);
                if (NULL != (item = cJSON_GetObjectItem(item_data, "signal")))
                    task_info->signal = item->valueint;
                if (NULL != (item = cJSON_GetObjectItem(item_data, "power")))
                    task_info->power = item->valueint;
                if (NULL != (item = cJSON_GetObjectItem(item_data, "retry")))
                    task_info->retry = item->valueint;
                if (NULL != (item = cJSON_GetObjectItem(item_data, "interval")))
                    task_info->interval = item->valueint;
#else
                if (NULL != (item = cJSON_GetObjectItem(item_data, "tid")))
                    task_info->tid = item->valueint;
#endif
            }
        }
        cJSON_Delete(root);
    }
    return ret;
}

#ifndef CONFIG_TM
int32_t ota_init(ota_evt_cb evt_cb, const uint8_t *dev_token, const uint8_t *dev_id, const uint8_t *version,
                 const uint8_t *manuf, const uint8_t *model, uint8_t type, boolean cdn)
#else
int32_t ota_init(ota_evt_cb evt_cb, const uint8_t *product_id, const uint8_t *dev_name, const uint8_t *access_key,
                 const uint8_t *version, uint8_t type)
#endif
{
    uint32_t malloc_buf_len = 0;

    if (NULL == (ota_ctx = osl_malloc(sizeof(struct ota_contex_t))))
    {
        return ERR_NO_MEM;
    }
    osl_memset(ota_ctx, 0, sizeof(struct ota_contex_t));

    malloc_buf_len = osl_strlen(version) + 1;
#ifndef CONFIG_TM
    malloc_buf_len += osl_strlen(dev_token) + 1;
    malloc_buf_len += osl_strlen(dev_id) + 1;
    malloc_buf_len += osl_strlen(manuf) + 1;
    malloc_buf_len += osl_strlen(model) + 1;
#else
    malloc_buf_len += 256;
    malloc_buf_len += osl_strlen(product_id) + 1;
    malloc_buf_len += osl_strlen(dev_name) + 1;
#endif
    if (NULL == (ota_ctx->dev_info.version = osl_malloc(malloc_buf_len)))
    {
        goto exit1;
    }
    osl_memset(ota_ctx->dev_info.version, 0, malloc_buf_len);

    osl_strcpy(ota_ctx->dev_info.version, version);

#ifndef CONFIG_TM
    ota_ctx->dev_info.dev_token = ota_ctx->dev_info.version + osl_strlen(ota_ctx->dev_info.version) + 1;
    osl_strcpy(ota_ctx->dev_info.dev_token, dev_token);

    ota_ctx->dev_info.dev_id = ota_ctx->dev_info.dev_token + osl_strlen(ota_ctx->dev_info.dev_token) + 1;
    osl_strcpy(ota_ctx->dev_info.dev_id, dev_id);

    ota_ctx->dev_info.manuf = ota_ctx->dev_info.dev_id + osl_strlen(ota_ctx->dev_info.dev_id) + 1;
    osl_strcpy(ota_ctx->dev_info.manuf, manuf);

    ota_ctx->dev_info.model = ota_ctx->dev_info.manuf + osl_strlen(ota_ctx->dev_info.manuf) + 1;
    osl_strcpy(ota_ctx->dev_info.model, model);

    ota_ctx->dev_info.cdn = cdn;
#else
    ota_ctx->dev_info.dev_token = ota_ctx->dev_info.version + osl_strlen(ota_ctx->dev_info.version) + 1;
    dev_token_generate(ota_ctx->dev_info.dev_token, SIG_METHOD_SHA1, 2524579200, product_id, dev_name, access_key);
    log_info("OTA calc token: %s\n", ota_ctx->dev_info.dev_token);

    ota_ctx->dev_info.product_id = ota_ctx->dev_info.dev_token + osl_strlen(ota_ctx->dev_info.dev_token) + 1;
    osl_strcpy(ota_ctx->dev_info.product_id, product_id);

    ota_ctx->dev_info.dev_name = ota_ctx->dev_info.product_id + osl_strlen(ota_ctx->dev_info.product_id) + 1;
    osl_strcpy(ota_ctx->dev_info.dev_name, dev_name);
#endif
    ota_ctx->dev_info.type = type;
    ota_ctx->evt_cb        = evt_cb;

    return ERR_OK;

exit1:
    osl_free(ota_ctx);
    return ERR_NO_MEM;
}

void ota_deinit(void)
{
    ota_free(ota_ctx->dev_info.version);
    ota_free(ota_ctx);
}

static void construct_general_header(void *http_ctx)
{
#ifndef CONFIG_TM
    http_add_param(http_ctx, "dev_id", ota_ctx->dev_info.dev_id);
#endif
    http_add_header(http_ctx, "Content-Type", "application/json");
    http_add_header(http_ctx, "Authorization", ota_ctx->dev_info.dev_token);
}

int32_t ota_post_version(uint32_t timeout_ms)
{
    void *   http_ctx      = NULL;
    uint8_t *resp_data     = NULL;
    uint32_t resp_data_len = 0;
    cJSON *  root          = NULL;
    uint8_t *content       = NULL;
    int32_t  ret           = ERR_NO_MEM;

// creat http contex
#ifdef CONFIG_TM
    uint8_t abs_path[64] = {0};

    osl_sprintf(abs_path, "/ota/%s/%s/version", ota_ctx->dev_info.product_id, ota_ctx->dev_info.dev_name);
    http_ctx = http_new(HTTP_METHOD_POST, OTA_HOST_ADDR, (const uint8_t *)abs_path, HTTP_PACK_SIZE_LEN);
#else
    http_ctx = http_new(HTTP_METHOD_POST, OTA_HOST_ADDR, "/ota/device/version", HTTP_PACK_SIZE_LEN);
#endif
    if (NULL == http_ctx)
    {
        return ret;
    }

    // add param and header
    construct_general_header(http_ctx);

    // add body
    if (NULL == (root = cJSON_CreateObject()))
    {
        goto exit;
    }
    // if (1 == ota_ctx->dev_info.type)
    {
        cJSON_AddStringToObject(root, "f_version", ota_ctx->dev_info.version);
    }
    // else
    {
        cJSON_AddStringToObject(root, "s_version", ota_ctx->dev_info.version);
    }
    if (NULL != (content = cJSON_PrintUnformatted(root)))
    {
        http_add_body(http_ctx, content, osl_strlen(content));
        osl_free(content);
    }
    cJSON_Delete(root);

    // send http request
#ifdef CONFIG_GENERAL_OTA_TLS
    ret = https_send_request(http_ctx, OTA_SERVER_ADDR, OTA_SERVER_PORT, ota_ca_cert, osl_strlen(ota_ca_cert),
                             &resp_data, &resp_data_len, timeout_ms);
#else
    ret = http_send_request(http_ctx, OTA_SERVER_ADDR, OTA_SERVER_PORT, &resp_data, &resp_data_len, timeout_ms);
#endif

    // parse resp data
    if (200 <= ret && 300 > ret)
    {
        ret = ota_common_resp_parse(resp_data, resp_data_len);
        ota_ctx->evt_cb(OTA_EVT_POST_VERSION, resp_data, resp_data_len);
    }

exit:
    http_delete(http_ctx);
    return ret;
}

int32_t ota_check_task(struct ota_task_info_t *task_info, uint32_t timeout_ms)
{
    void *   http_ctx      = NULL;
    uint8_t *resp_data     = NULL;
    uint32_t resp_data_len = 0;
    int32_t  ret           = ERR_NO_MEM;

// creat http contex
#ifdef CONFIG_TM
    uint8_t abs_path[64] = {0};

    osl_sprintf(abs_path, "/ota/%s/%s/check", ota_ctx->dev_info.product_id, ota_ctx->dev_info.dev_name);
    http_ctx = http_new(HTTP_METHOD_GET, OTA_HOST_ADDR, (const uint8_t *)abs_path, HTTP_PACK_SIZE_LEN);
#else
    http_ctx = http_new(HTTP_METHOD_GET, OTA_HOST_ADDR, "/ota/south/check", HTTP_PACK_SIZE_LEN);
#endif
    if (NULL == http_ctx)
    {
        return ret;
    }

    if (1 == ota_ctx->dev_info.type)
    {
        http_add_param(http_ctx, "type", "1");
    }
    else
    {
        http_add_param(http_ctx, "type", "2");
    }
    http_add_param(http_ctx, "version", ota_ctx->dev_info.version);
#ifndef CONFIG_TM
    if (ota_ctx->dev_info.cdn)
    {
        http_add_param(http_ctx, "cdn", "true");
    }
    else
    {
        http_add_param(http_ctx, "cdn", "false");
    }
#endif
    // add param and header
    construct_general_header(http_ctx);

    // send http request
#ifdef CONFIG_GENERAL_OTA_TLS
    ret = https_send_request(http_ctx, OTA_SERVER_ADDR, OTA_SERVER_PORT, ota_ca_cert, osl_strlen(ota_ca_cert),
                             &resp_data, &resp_data_len, timeout_ms);
#else
    ret = http_send_request(http_ctx, OTA_SERVER_ADDR, OTA_SERVER_PORT, &resp_data, &resp_data_len, timeout_ms);
#endif

    // parse resp data
    if (200 <= ret && 300 > ret)
    {
        ret = ota_check_task_parse(task_info, resp_data, resp_data_len);
        ota_ctx->evt_cb(OTA_EVT_CHECK_TASK, resp_data, resp_data_len);
    }

    http_delete(http_ctx);
    return ret;
}

#ifndef CONFIG_TM
int32_t ota_check_token(uint8_t *token, uint32_t timeout_ms)
#else
int32_t ota_check_token(uint32_t tid, uint32_t timeout_ms)
#endif
{
    void *   http_ctx      = NULL;
    uint8_t *resp_data     = NULL;
    uint32_t resp_data_len = 0;
    uint8_t  abs_path[64]  = {0};
    int32_t  ret           = ERR_NO_MEM;

// creat http contex
#ifdef CONFIG_TM
    osl_sprintf(abs_path, (const uint8_t *)"/ota/%s/%s/%d/check", ota_ctx->dev_info.product_id,
                ota_ctx->dev_info.dev_name, tid);
#else
    osl_sprintf(abs_path, (const uint8_t *)"/ota/south/download/%s/check", token);
#endif
    http_ctx = http_new(HTTP_METHOD_GET, OTA_HOST_ADDR, abs_path, HTTP_PACK_SIZE_LEN);
    if (NULL == http_ctx)
    {
        return ret;
    }

    // add param and header
    construct_general_header(http_ctx);

    // send http request
#ifdef CONFIG_GENERAL_OTA_TLS
    ret = https_send_request(http_ctx, OTA_SERVER_ADDR, OTA_SERVER_PORT, ota_ca_cert, osl_strlen(ota_ca_cert),
                             &resp_data, &resp_data_len, timeout_ms);
#else
    ret = http_send_request(http_ctx, OTA_SERVER_ADDR, OTA_SERVER_PORT, &resp_data, &resp_data_len, timeout_ms);
#endif

    // parse resp data
    if (200 <= ret && 300 > ret)
    {
        ret = ota_common_resp_parse(resp_data, resp_data_len);
        ota_ctx->evt_cb(OTA_EVT_CHECK_TOKEN, resp_data, resp_data_len);
    }

    http_delete(http_ctx);
    return ret;
}

#ifndef CONFIG_TM
int32_t ota_download_package(uint8_t *token, uint32_t range_start, uint32_t range_size, uint32_t timeout_ms)
#else
int32_t ota_download_package(uint32_t tid, uint32_t range_start, uint32_t range_size, uint32_t timeout_ms)
#endif
{
    void *   http_ctx         = NULL;
    uint8_t *resp_data        = NULL;
    uint32_t resp_data_len    = 0;
    uint8_t  abs_path[64]     = {0};
    uint8_t  range_header[32] = {0};
    int32_t  ret              = ERR_NO_MEM;

// creat http contex
#ifdef CONFIG_TM
    osl_sprintf(abs_path, (const uint8_t *)"/ota/%s/%s/%d/download/", ota_ctx->dev_info.product_id,
                ota_ctx->dev_info.dev_name, tid);
#else
    osl_sprintf(abs_path, (const uint8_t *)"/ota/south/download/%s", token);
#endif
    http_ctx = http_new(HTTP_METHOD_GET, OTA_HOST_ADDR, abs_path, range_size + HTTP_PACK_SIZE_LEN);
    if (NULL == http_ctx)
    {
        return ret;
    }

    // add param and header
    http_add_header(http_ctx, "Authorization", ota_ctx->dev_info.dev_token);
    
    osl_sprintf(range_header, (const uint8_t *)"%d-%d", range_start, range_start + range_size - 1);
    http_add_header(http_ctx, "Range", range_header);

    log_info("ota download range: %s\n", range_header);

    // send http request
#ifdef CONFIG_GENERAL_OTA_TLS
    ret = https_send_request(http_ctx, OTA_SERVER_ADDR, OTA_SERVER_PORT, ota_ca_cert, osl_strlen(ota_ca_cert),
                             &resp_data, &resp_data_len, timeout_ms);
#else
    ret = http_send_request(http_ctx, OTA_SERVER_ADDR, OTA_SERVER_PORT, &resp_data, &resp_data_len, timeout_ms);
#endif

    // parse resp data
    if (200 <= ret && 300 > ret)
    {
        if (ERR_OK != http_get_resp_header(http_ctx, "Content-Length", NULL))
        {
            ret = *resp_data; //下载失败，返回错误码
            goto exit;
        }
        ota_ctx->evt_cb(OTA_EVT_DOWNLOAD_PACKAGE, resp_data, resp_data_len);
        ret = 0;
    }

exit:
    http_delete(http_ctx);
    return ret;
}

#ifndef CONFIG_TM
int32_t ota_report_status(uint8_t *token, uint32_t status_code, uint32_t timeout_ms)
#else
int32_t ota_report_status(uint32_t tid, uint32_t status_code, uint32_t timeout_ms)
#endif
{
    void *   http_ctx      = NULL;
    uint8_t *resp_data     = NULL;
    uint32_t resp_data_len = 0;
    uint8_t  abs_path[64]  = {0};
    cJSON *  root          = NULL;
    uint8_t *content       = NULL;
    int32_t  ret           = ERR_NO_MEM;

// creat http contex
#ifdef CONFIG_TM
    osl_sprintf(abs_path, (const uint8_t *)"/ota/%s/%s/%d/status", ota_ctx->dev_info.product_id,
                ota_ctx->dev_info.dev_name, tid);
#else
    osl_sprintf(abs_path, (const uint8_t *)"/ota/south/device/download/%s/progress", token);
#endif
    http_ctx = http_new(HTTP_METHOD_POST, OTA_HOST_ADDR, abs_path, HTTP_PACK_SIZE_LEN);
    if (NULL == http_ctx)
    {
        return ret;
    }

    // add param and header
    construct_general_header(http_ctx);

    // add body
    if (NULL == (root = cJSON_CreateObject()))
    {
        goto exit;
    }
    cJSON_AddNumberToObject(root, "step", status_code);
    if (NULL != (content = cJSON_PrintUnformatted(root)))
    {
        http_add_body(http_ctx, content, osl_strlen(content));
        osl_free(content);
    }
    cJSON_Delete(root);

    // send http request
#ifdef CONFIG_GENERAL_OTA_TLS
    ret = https_send_request(http_ctx, OTA_SERVER_ADDR, OTA_SERVER_PORT, ota_ca_cert, osl_strlen(ota_ca_cert),
                             &resp_data, &resp_data_len, timeout_ms);
#else
    ret = http_send_request(http_ctx, OTA_SERVER_ADDR, OTA_SERVER_PORT, &resp_data, &resp_data_len, timeout_ms);
#endif

    // parse resp data
    if (200 <= ret && 300 > ret)
    {
        ret = ota_common_resp_parse(resp_data, resp_data_len);
        ota_ctx->evt_cb(OTA_EVT_REPORT_STATUS, resp_data, resp_data_len);
    }

exit:
    http_delete(http_ctx);
    return ret;
}
