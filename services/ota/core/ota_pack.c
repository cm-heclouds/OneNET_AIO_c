/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "ota_api.h"
#include "ota_internal.h"

#include "http_parser.h"

/*
 * adapter
 */
#include "plat_tcp.h"
#include "ota_sys.h"

#include "ota_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/
/* Local Definitions ( Constant and Macro)                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/
static void *_OTA_Malloc_Packet_(uint16_t buffer_len);
static void _OTA_Pack_Common_Header_(char *buffer, char *authorization);
static void _OTA_Pack_Length_(char *buffer, uint16_t len);
/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/

uint8_t otaPack_Submit_Version(ota_context *ctx, uint16_t buffer_len)
{
    uint16_t body_len = 0;
    char *packet_ptr = (char *)_OTA_Malloc_Packet_(buffer_len);

    if(packet_ptr == NULL)
    {
        return OTA_ERROR;
    }
    ctx->net.SendBuffer = packet_ptr;
    body_len = ctx->device_info.s_version_len + ctx->device_info.f_version_len + 31;
    if(ctx->device_info.s_version_len == 0 || ctx->device_info.f_version_len == 0)
    {
        body_len -= 15;
    }

    ota_sprintf(packet_ptr, "POST /ota/device/version?%s=%s", OTA_HTTP_HEADER_DEVICE_FIELD,
                ctx->device_info.dev_id);

    _OTA_Pack_Common_Header_(packet_ptr + ota_strlen(packet_ptr), ctx->device_info.authorization);
    _OTA_Pack_Length_(packet_ptr + ota_strlen(packet_ptr), body_len);

    ota_strcat(packet_ptr, "{");

    if(ctx->device_info.f_version_len != 0)
    {
        ota_sprintf(packet_ptr + ota_strlen(packet_ptr), "\"f_version\":\"%s\"",
                    ctx->device_info.f_version);
    }
    if(ctx->device_info.s_version_len != 0)
    {
        if(ctx->device_info.f_version_len != 0)
        {
            ota_strcat(packet_ptr, ",");
        }
        ota_sprintf(packet_ptr + ota_strlen(packet_ptr), "\"s_version\":\"%s\"",
                    ctx->device_info.s_version);
    }

    ota_strcat(packet_ptr, "}");
    return OTA_OK;
}

uint8_t otaPack_Check_Task(ota_context *ctx, uint16_t buffer_len)
{
    char *packet_ptr = (char *)_OTA_Malloc_Packet_(buffer_len);
    char *version_ptr = NULL;

    if(packet_ptr == NULL)
    {
        return OTA_ERROR;
    }
    ctx->net.SendBuffer = packet_ptr;

    if(ctx->device_info.ota_type == 1)
    {
        version_ptr = ctx->device_info.f_version;
    }
    else
    {
        version_ptr = ctx->device_info.s_version;
    }

    ota_sprintf(packet_ptr, "GET /ota/south/check?%s=%s&manuf=%s&model=%s&type=%d&version=%s",
                OTA_HTTP_HEADER_DEVICE_FIELD, ctx->device_info.dev_id, OTA_MANUF, OTA_MODEL,
                ctx->device_info.ota_type, version_ptr);

    _OTA_Pack_Common_Header_(packet_ptr + ota_strlen(packet_ptr), ctx->device_info.authorization);
    ota_strcat(packet_ptr, "\r\n");

    return OTA_OK;
}

uint8_t otaPack_Validate_Token(ota_context *ctx, uint16_t buffer_len)
{
    char *packet_ptr = (char *)_OTA_Malloc_Packet_(buffer_len);

    if(packet_ptr == NULL)
    {
        return OTA_ERROR;
    }
    ctx->net.SendBuffer = packet_ptr;

    ota_sprintf(packet_ptr, "GET /ota/south/download/%s/check?%s=%s", ctx->task_info.token,
                OTA_HTTP_HEADER_DEVICE_FIELD, ctx->device_info.dev_id);

    _OTA_Pack_Common_Header_(packet_ptr + ota_strlen(packet_ptr), ctx->device_info.authorization);
    ota_strcat(packet_ptr, "\r\n");

    return OTA_OK;
}

uint8_t otaPack_Download_Package(ota_context *ctx, char *bs_id, uint16_t buffer_len)
{
    char *packet_ptr = (char *)_OTA_Malloc_Packet_(buffer_len);

    if(packet_ptr == NULL)
    {
        return OTA_ERROR;
    }
    ctx->net.SendBuffer = packet_ptr;

    ota_sprintf(packet_ptr, "GET /ota/south/download/%s", ctx->task_info.token);
    if(bs_id != NULL)
    {
        ota_sprintf(packet_ptr + ota_strlen(packet_ptr), "?s_id=%s", bs_id);
    }
    _OTA_Pack_Common_Header_(packet_ptr + ota_strlen(packet_ptr), ctx->device_info.authorization);

    ota_sprintf(packet_ptr + ota_strlen(packet_ptr), "Range: bytes=%d-%d\r\n\r\n",
                ctx->download_info.range_next_start, ctx->download_info.range_next_end);

    return OTA_OK;
}

uint8_t otaPack_Update_Progress(ota_context *ctx, uint16_t buffer_len)
{
    uint16_t _step_ = 0;
    uint16_t body_len = 0;
    char *packet_ptr = (char *)_OTA_Malloc_Packet_(buffer_len);

    if(packet_ptr == NULL)
    {
        return OTA_ERROR;
    }
    ctx->net.SendBuffer = packet_ptr;

    ota_sprintf(packet_ptr, "POST /ota/south/device/download/%s/progress?%s=%s",
                ctx->task_info.token, OTA_HTTP_HEADER_DEVICE_FIELD, ctx->device_info.dev_id);

    _OTA_Pack_Common_Header_(packet_ptr + ota_strlen(packet_ptr), ctx->device_info.authorization);

    _step_ = ctx->download_info.download_progress;
    if(ctx->download_info.download_progress < 10 &&
       ctx->state == CTX_STATE_REPORT_DOWNLOAD_PROGRESS)
    {
        body_len = 10;
    }
    else if(ctx->download_info.download_progress != 100 &&
            ctx->state == CTX_STATE_REPORT_DOWNLOAD_PROGRESS)
    {
        body_len = 11;
    }
    else if((ctx->download_info.download_progress == 100 &&
             ctx->state == CTX_STATE_REPORT_DOWNLOAD_PROGRESS) ||
            ctx->state == CTX_STATE_REPORT)
    {
        body_len = 12;
        if(ctx->state == CTX_STATE_REPORT)
        {
            _step_ = ctx->report_code;
        }
    }
    else
    {
        return OTA_ERROR;
    }
    _OTA_Pack_Length_(packet_ptr + ota_strlen(packet_ptr), body_len);

    ota_sprintf(packet_ptr + ota_strlen(packet_ptr), "{\"step\":%d}", _step_);

    return OTA_OK;
}

static void *_OTA_Malloc_Packet_(uint16_t buffer_len)
{
    void *ptr = ota_malloc(buffer_len);
    if(ptr != NULL)
    {
        ota_memset(ptr, 0, buffer_len);
    }
    else
    {
        ota_log_printf("malloc error");
        return NULL;
    }

    return ptr;
}

static void _OTA_Pack_Common_Header_(char *buffer, char *authorization)
{
    ota_sprintf(buffer,
                " HTTP/1.1\r\n"
                "host: %s\r\n"
                "Content-Type: application/json\r\n"
                "Connection: keep-alive\r\n",
                OTA_HOST);
    if(authorization != NULL)
    {
        ota_sprintf(buffer + ota_strlen(buffer), "authorization: %s\r\n", authorization);
    }
}

static void _OTA_Pack_Length_(char *buffer, uint16_t len)
{
    ota_sprintf(buffer, "Content-Length: %d\r\n\r\n", len);
}