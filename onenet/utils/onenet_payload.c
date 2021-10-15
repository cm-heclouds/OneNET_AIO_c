/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file onenet_payload.c
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include <time.h>

#include "plat_osl.h"
#include "utils.h"
#include "onenet_payload.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define ONENET_DATA_TYPE3_NUM_FMT "{\"%s\":%xxx}"
#define ONENET_DATA_TYPE4_NUM_FMT "{\"%s\":{\"%s\":%xxx}}"

#define ONENET_DATA_TYPE3_STR_FMT "{\"%s\":\"%s\"}"
#define ONENET_DATA_TYPE4_STR_FMT "{\"%s\":{\"%s\":\"%s\"}}"

#define ONENET_DATA_TYPE2_DESC_FMT           "{\"ds_id\":\"%s\"}"
#define ONENET_DATA_TYPE2_DESC_WITH_TIME_FMT "{\"ds_id\":\"%s\",\"at\":\"%s\"}"

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static uint8_t ts_str[24];
/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
static uint8_t *ts2str(uint32_t timestamp)
{
    struct tm *tm  = NULL;
    time_t     tmp = timestamp;

    tm = localtime(&tmp);
    osl_sprintf(ts_str, (const uint8_t *)"%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                tm->tm_hour, tm->tm_min, tm->tm_sec);
    return ts_str;
}

static uint32_t construct_type2(uint8_t *payload, struct iot_data_t *data)
{
    uint32_t ret    = 0;
    uint32_t offset = 0;

    payload[offset++] = 2;

    if (data->timestamp)
    {
        ret = osl_sprintf(payload + offset + 2, (const uint8_t *)ONENET_DATA_TYPE2_DESC_WITH_TIME_FMT, data->data_id,
                          ts2str(data->timestamp));
    }
    else
    {
        ret = osl_sprintf(payload + offset + 2, (const uint8_t *)ONENET_DATA_TYPE2_DESC_FMT, data->data_id);
    }
    offset += set_16bit_be(payload + offset, ret);
    offset += ret;

    offset += set_32bit_be(payload + offset, data->buf.val_bin.buf_len);
    osl_memcpy(payload + offset, data->buf.val_bin.buf, data->buf.val_bin.buf_len);

    return (offset + data->buf.val_bin.buf_len);
}

static uint32_t construct_type3(uint8_t *payload, struct iot_data_t *data)
{
    uint8_t  fmt[16] = {0};
    int32_t  ret     = 0;
    uint32_t offset  = 0;

    payload[offset++] = 3;
    osl_strcpy(fmt, (const uint8_t *)ONENET_DATA_TYPE3_NUM_FMT);

    if (data->data_type == IOT_DATA_TYPE_INT)
    {
        osl_memcpy(fmt + 7, "llu", 3);
        ret = osl_sprintf(payload + offset + 2, fmt, data->data_id, data->buf.val_int);
    }
    else if (data->data_type == IOT_DATA_TYPE_FLOAT)
    {
        osl_memcpy(fmt + 7, ".6f", 3);
        ret = osl_sprintf(payload + offset + 2, fmt, data->data_id, data->buf.val_float);
    }
    else
    {
        ret = osl_sprintf(payload + offset + 2, (const uint8_t *)ONENET_DATA_TYPE3_STR_FMT, data->data_id,
                          data->buf.val_str);
    }
    offset += set_16bit_be(payload + offset, ret);

    if (0 < ret)
    {
        return (offset + ret);
    }
    else
    {
        return 0;
    }
}

static uint32_t construct_type4(uint8_t *payload, struct iot_data_t *data)
{
    uint8_t  fmt[20] = {0};
    int32_t  ret     = 0;
    uint32_t offset  = 0;

    payload[offset++] = 4;
    osl_strcpy(fmt, (const uint8_t *)ONENET_DATA_TYPE4_NUM_FMT);

    if (data->data_type == IOT_DATA_TYPE_INT)
    {
        osl_memcpy(fmt + 13, "llu", 3);
        ret = osl_sprintf(payload + offset + 2, fmt, data->data_id, ts2str(data->timestamp), data->buf.val_int);
    }
    else if (data->data_type == IOT_DATA_TYPE_FLOAT)
    {
        osl_memcpy(fmt + 13, ".6f", 3);
        ret = osl_sprintf(payload + offset + 2, fmt, data->data_id, ts2str(data->timestamp), data->buf.val_float);
    }
    else
    {
        ret = osl_sprintf(payload + offset + 2, (const uint8_t *)ONENET_DATA_TYPE4_STR_FMT, data->data_id,
                          ts2str(data->timestamp), data->buf.val_str);
    }
    offset += set_16bit_be(payload + offset, ret);

    if (0 < ret)
    {
        return (offset + ret);
    }
    else
    {
        return 0;
    }
}

uint32_t iot_data_to_payload(uint8_t *payload, struct iot_data_t *data)
{
    if (data->data_type == IOT_DATA_TYPE_BINARY)
    {
        return construct_type2(payload, data);
    }
    else if (data->timestamp)
    {
        return construct_type4(payload, data);
    }
    else
    {
        return construct_type3(payload, data);
    }
}
