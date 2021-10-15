/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file iot_edp.c
 * @date 2020/07/20
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include <stdio.h>

#include "iot_edp.h"
#include "iot_api.h"
#include "onenet_payload.h"

#include "plat_osl.h"
#include "plat_tcp.h"
#include "plat_time.h"
#include "utils.h"
#include "err_def.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define IOT_EDP_SERVER_ADDR "183.230.40.39"
#define IOT_EDP_SERVER_PORT 876

#define EDP_PROTOCOL_STR "EDP"
#define EDP_PROTOCOL_VER 1

#define EDP_REMAIN_LENGTH_MAX_BYTES 4

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
#define EDP_MSG_TYPE_NONE         0
#define EDP_MSG_TYPE_CONN_REQ     1
#define EDP_MSG_TYPE_CONN_RESP    2
#define EDP_MSG_TYPE_PUSH_DATA    3
#define EDP_MSG_TYPE_CONN_CLOSE   4
#define EDP_MSG_TYPE_UPDATE_REQ   5
#define EDP_MSG_TYPE_UPDATE_RESP  6
#define EDP_MSG_TYPE_SUB_DEVICE   7
#define EDP_MSG_TYPE_SAVE_DATA    8
#define EDP_MSG_TYPE_SAVE_ACK     9
#define EDP_MSG_TYPE_CMD_REQ      10
#define EDP_MSG_TYPE_CMD_RESP     11
#define EDP_MSG_TYPE_PING_REQ     12
#define EDP_MSG_TYPE_PING_RESP    13
#define EDP_MSG_TYPE_ENCRYPT_REQ  14
#define EDP_MSG_TYPE_ENCRYPT_RESP 15

struct iot_edp_t
{
    handle_t         conn_fd;
    iot_dev_event_cb evt_cb;
    uint8_t *        send_buf;
    uint8_t *        recv_buf;
    uint32_t         life_time;
    uint64_t         last_send_time;
    uint64_t         last_ping_send_time;
};

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
#ifdef CONFIG_IOT_EDP_SAVE_ACK
static uint16_t edp_msg_id = 1;
#endif
/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
static uint32_t pack_message_type(uint8_t *buf, uint8_t msg_type)
{
    *buf = msg_type << 4;
    return 1;
}

static uint32_t pack_remain_length(uint8_t *buf, uint32_t remain_len)
{
    uint32_t len    = remain_len;
    uint32_t offset = 0;

    do
    {
        buf[offset++] = (len & 0x7F) | 0x80;
        len >>= 7;
    } while (len > 0);

    buf[offset - 1] &= 0x7F;
    return offset;
}

static uint32_t pack_data(uint8_t *buf, uint8_t *data, uint32_t data_len)
{
    uint32_t offset = 0;

    offset += set_16bit_be(buf, data_len);
    if (data_len)
        osl_memcpy(buf + offset, data, data_len);

    return (offset + data_len);
}

static uint32_t unpack_data(uint8_t *buf, uint8_t *data, uint32_t *data_len)
{
    uint32_t offset = 0;
    uint16_t len    = 0;

    offset += get_16bit_be(buf + offset, &len);
    *data_len = len;
    osl_memcpy(data, buf + offset, *data_len);
    return (offset + *data_len);
}

static int32_t process_conn_resp(uint8_t *buf)
{
    if (0 == buf[1])
        return ERR_OK;
    else
        return ERR_CLOUD;
}

static int32_t process_push_ack(struct iot_edp_t *dev, uint8_t *buf, uint32_t buf_len)
{
    struct iot_dev_cmd_t dev_cmd;
    uint32_t             data_src_id_len = buf[0] << 8 | buf[1];

    dev_cmd.data_id  = osl_strndup(buf + 2, data_src_id_len);
    dev_cmd.data     = buf + 2 + data_src_id_len;
    dev_cmd.data_len = buf_len - 2 - data_src_id_len;
    dev->evt_cb(IOT_DEV_RESOURCE_PUSHED, &dev_cmd, sizeof(dev_cmd));
    osl_free((void *)dev_cmd.data_id);
    return ERR_OK;
}

static int32_t process_cmd(struct iot_edp_t *dev, uint8_t *buf, handle_t cd_tmr)
{
    struct iot_dev_cmd_t dev_cmd      = {0};
    uint32_t             cmd_id_len   = buf[0] << 8 | buf[1];
    uint32_t             cmd_data_len = 0;
    uint32_t             send_len     = 0;
    int32_t              ret          = ERR_OK;

    dev_cmd.data_id = osl_strndup(buf + 2, cmd_id_len);
    cmd_data_len    = buf[2 + cmd_id_len] << 24 | buf[2 + cmd_id_len + 1] << 16 | buf[2 + cmd_id_len + 2] << 8
                   | buf[2 + cmd_id_len + 3];
    dev_cmd.data     = buf + 6 + cmd_id_len;
    dev_cmd.data_len = cmd_data_len;
    dev_cmd.resp     = buf + EDP_REMAIN_LENGTH_MAX_BYTES + 6 + cmd_id_len;
    dev_cmd.resp_len = 0;
    if (ERR_OK == dev->evt_cb(IOT_DEV_CMD_RECEIVED, &dev_cmd, sizeof(dev_cmd)))
    {
        uint32_t offset = 0;
        offset += pack_message_type(dev->send_buf, EDP_MSG_TYPE_CMD_RESP);
        offset += pack_remain_length(dev->send_buf + offset, 6 + cmd_id_len + dev_cmd.resp_len);
        offset += set_16bit_be(dev->send_buf + offset, cmd_id_len);
        osl_strcpy(dev->send_buf + offset, dev_cmd.data_id);
        offset += cmd_id_len;
        offset += set_32bit_be(dev->send_buf + offset, dev_cmd.resp_len);
        offset += dev_cmd.resp_len;
        send_len = offset;
        if (send_len != plat_tcp_send(dev->conn_fd, dev->send_buf, offset, countdown_left(cd_tmr)))
            ret = ERR_NETWORK;
        else
            dev->last_send_time = time_count();
    }
    osl_free((void *)dev_cmd.data_id);
    return ret;
}

#ifdef CONFIG_IOT_EDP_SAVE_ACK
static int32_t process_save_ack(struct iot_edp_t *dev, uint8_t *buf, uint32_t buf_len)
{
    uint16_t msg_id = 0;

    if (0x40 != buf[0])
        return ERR_INVALID_DATA;
    msg_id = buf[1] << 8 | buf[2];
    if (msg_id == edp_msg_id)
    {
        if (0 == buf[3])
            return ERR_OK;
        else
            return ERR_CLOUD;
    }
    else
        /** Ignore*/
        return ERR_TIMEOUT;
}
#endif

static int32_t edp_parse_data(struct iot_edp_t *dev, uint8_t *msg_type, handle_t cd_tmr)
{
    uint32_t recv_len   = 0;
    uint32_t remain_len = 0;
    uint32_t offset     = 0;
    uint32_t i          = 0;
    int32_t  ret        = ERR_OK;

    recv_len = plat_tcp_recv(dev->conn_fd, dev->recv_buf, 1, countdown_left(cd_tmr));
    if (0 == recv_len)
        return ERR_OK;
    if (0 > recv_len)
        return ERR_NETWORK;
    if (dev->recv_buf[0] & 0x0F)
        return ERR_INVALID_DATA;
    *msg_type = *dev->recv_buf >> 4;

    do
    {
        recv_len = plat_tcp_recv(dev->conn_fd, dev->recv_buf, 1, countdown_left(cd_tmr));
        remain_len |= (*dev->recv_buf & 0x7F) << (7 * i++);
    } while ((*dev->recv_buf) >> 7);

    if (remain_len > CONFIG_RECV_BUF_LEN_MAX)
        return ERR_NO_MEM;
    if (remain_len)
    {
        recv_len = plat_tcp_recv(dev->conn_fd, dev->recv_buf, remain_len, countdown_left(cd_tmr));
        if (recv_len != remain_len)
            return ERR_NETWORK;
    }

    switch (*msg_type)
    {
        case EDP_MSG_TYPE_CONN_RESP:
            ret = process_conn_resp(dev->recv_buf);
            break;
        case EDP_MSG_TYPE_CONN_CLOSE:
            ret = ERR_CLOUD;
            break;
        case EDP_MSG_TYPE_PUSH_DATA:
            ret = process_push_ack(dev, dev->recv_buf, remain_len);
            break;
        case EDP_MSG_TYPE_CMD_REQ:
            ret = process_cmd(dev, dev->recv_buf, cd_tmr);
            break;
        case EDP_MSG_TYPE_PING_RESP:
            dev->last_send_time = dev->last_ping_send_time;
            break;
#ifdef CONFIG_IOT_EDP_SAVE_ACK
        case EDP_MSG_TYPE_SAVE_ACK:
            break;
#endif
        default:
            ret = ERR_NOT_SUPPORT;
            break;
    }
    return ret;
}

static uint32_t pack_packet(uint8_t *buf, uint8_t msg_type, uint8_t *remain_data, uint32_t remain_len)
{
    uint32_t offset = 0;

    offset += pack_message_type(buf, msg_type);
    offset += pack_remain_length(buf + offset, remain_len);
    if (remain_len)
        osl_memmove(buf + offset, remain_data, remain_len);

    return (offset + remain_len);
}

static uint32_t pack_payload_login(uint8_t *buf, const uint8_t *product_id, const uint8_t *device_sn,
                                   uint32_t life_time)
{
    uint32_t offset = 0;

    offset += pack_data(buf, (uint8_t *)EDP_PROTOCOL_STR, osl_strlen(EDP_PROTOCOL_STR));
    buf[offset++] = EDP_PROTOCOL_VER;
    buf[offset++] = 0xC0;
    offset += set_16bit_be(buf + offset, life_time);
    offset += pack_data(buf + offset, NULL, 0);
    offset += pack_data(buf + offset, (uint8_t *)product_id, osl_strlen(product_id));
    offset += pack_data(buf + offset, (uint8_t *)device_sn, osl_strlen(device_sn));

    return offset;
}

static uint32_t pack_payload_push_data(uint8_t *buf, const uint8_t *target, uint8_t *data, uint32_t data_len)
{
    uint32_t offset = 0;

    if (target)
        offset += pack_data(buf, (uint8_t *)target, osl_strlen(target));
    else
        offset += pack_data(buf, NULL, 0);

    osl_memmove(buf + offset, data, data_len);

    return (offset + data_len);
}

static uint32_t pack_payload_save_data(uint8_t *buf, struct iot_data_t *data)
{
    uint32_t offset = 0;
#ifdef CONFIG_IOT_EDP_SAVE_ACK
    buf[offset++] = 1 << 6;
    if (0 == edp_msg_id)
        edp_msg_id++;
    offset += set_16bit_be(buf + offset, edp_msg_id++);
#else
    buf[offset++] = 0;
#endif

    offset += iot_data_to_payload(buf + offset, data);

    return offset;
}

static uint32_t construct_login(uint8_t *buf, const uint8_t *product_id, const uint8_t *device_sn, uint32_t life_time)
{
    uint32_t remain_len  = 0;
    uint8_t *remain_data = buf + EDP_REMAIN_LENGTH_MAX_BYTES;

    remain_len = pack_payload_login(remain_data, product_id, device_sn, life_time);

    return pack_packet(buf, EDP_MSG_TYPE_CONN_REQ, remain_data, remain_len);
}

static uint32_t construct_ping(uint8_t *buf)
{
    return pack_packet(buf, EDP_MSG_TYPE_PING_REQ, NULL, 0);
}

static uint32_t construct_save_data(uint8_t *buf, struct iot_data_t *data)
{
    uint32_t remain_len  = 0;
    uint8_t *remain_data = buf + EDP_REMAIN_LENGTH_MAX_BYTES;

    remain_len = pack_payload_save_data(remain_data, data);

    return pack_packet(buf, EDP_MSG_TYPE_SAVE_DATA, remain_data, remain_len);
}

static uint32_t construct_push_data(uint8_t *buf, const uint8_t *target, uint8_t *data, uint32_t data_len)
{
    uint32_t remain_len  = 0;
    uint8_t *remain_data = buf + EDP_REMAIN_LENGTH_MAX_BYTES;

    remain_len = pack_payload_push_data(remain_data, target, data, data_len);

    return pack_packet(buf, EDP_MSG_TYPE_PUSH_DATA, remain_data, remain_len);
}

static int32_t edp_login(struct iot_edp_t *dev, const uint8_t *product_id, const uint8_t *device_sn, uint32_t life_time,
                         handle_t cd_tmr)
{
    uint32_t send_len       = 0;
    uint8_t  reply_msg_type = EDP_MSG_TYPE_NONE;
    int32_t  ret            = ERR_OK;

    dev->conn_fd = plat_tcp_connect(IOT_EDP_SERVER_ADDR, IOT_EDP_SERVER_PORT, countdown_left(cd_tmr));

    if (dev->conn_fd < 0)
        return ERR_NETWORK;

    send_len = construct_login(dev->send_buf, product_id, device_sn, life_time);
    if (send_len != plat_tcp_send(dev->conn_fd, dev->send_buf, send_len, countdown_left(cd_tmr)))
    {
        plat_tcp_disconnect(dev->conn_fd);
        return ERR_NETWORK;
    }
    dev->last_send_time = time_count();
    /** Wait Ack*/
    do
    {
        ret = edp_parse_data(dev, &reply_msg_type, cd_tmr);
        if((ERR_OK == ret) && (reply_msg_type == EDP_MSG_TYPE_CONN_RESP))
        {
            return ERR_OK;
        }
        else if(ERR_TIMEOUT != ret)
        {
            return ret;
        }
    } while (0 == countdown_is_expired(cd_tmr));

    return ERR_TIMEOUT;
}

static int32_t edp_logout(struct iot_edp_t *dev)
{
    plat_tcp_disconnect(dev->conn_fd);

    return ERR_OK;
}

static int32_t edp_send_ping(struct iot_edp_t *dev)
{
    uint32_t send_len = construct_ping(dev->send_buf);

    if (send_len != plat_tcp_send(dev->conn_fd, dev->send_buf, send_len, 0))
        return ERR_NETWORK;
    dev->last_ping_send_time = time_count();
    return ERR_OK;
}

static int32_t edp_save_data(struct iot_edp_t *dev, struct iot_data_t *data, handle_t cd_tmr)
{
    uint8_t  reply_msg_type = EDP_MSG_TYPE_NONE;
    uint32_t send_len       = construct_save_data(dev->send_buf, data);
    int32_t  ret            = ERR_OK;

    if (send_len != plat_tcp_send(dev->conn_fd, dev->send_buf, send_len, countdown_left(cd_tmr)))
        return ERR_NETWORK;
    dev->last_send_time = time_count();
#ifdef CONFIG_IOT_EDP_SAVE_ACK
    do
    {
        ret = edp_parse_data(dev, &reply_msg_type, cd_tmr);
        if ((reply_msg_type == EDP_MSG_TYPE_SAVE_ACK) && (ERR_TIMEOUT != ret))
        {
            break;
        }
    } while (0 == countdown_is_expired(cd_tmr));
#endif
    return ERR_OK;
}

static int32_t edp_push_data(struct iot_edp_t *dev, const uint8_t *target, uint8_t *data, uint32_t data_len,
                             handle_t cd_tmr)
{
    uint32_t send_len = construct_push_data(dev->send_buf, target, data, data_len);

    if (send_len != plat_tcp_send(dev->conn_fd, dev->send_buf, send_len, countdown_left(cd_tmr)))
        return ERR_NETWORK;
    dev->last_send_time = time_count();
    return ERR_OK;
}

void *iot_edp_open(struct iot_cloud_config_t *cloud, iot_dev_event_cb event_cb, uint32_t timeout_ms)
{
    handle_t          cd      = countdown_start(timeout_ms);
    struct iot_edp_t *edp_dev = NULL;

    edp_dev = osl_malloc(sizeof(*edp_dev));
    if (NULL == edp_dev)
        goto exit;

    edp_dev->recv_buf = osl_malloc(CONFIG_RECV_BUF_LEN_MAX);
    if (NULL == edp_dev->recv_buf)
        goto exit1;
    osl_memset(edp_dev->recv_buf, 0, CONFIG_RECV_BUF_LEN_MAX);

    edp_dev->send_buf = osl_malloc(CONFIG_SEND_BUF_LEN_MAX);
    if (NULL == edp_dev->send_buf)
        goto exit2;
    osl_memset(edp_dev->send_buf, 0, CONFIG_SEND_BUF_LEN_MAX);

    edp_dev->life_time = cloud->life_time;
    if (0 != edp_login(edp_dev, cloud->product_id, cloud->device_sn, cloud->life_time, cd))
        goto exit3;
    edp_dev->evt_cb = event_cb;
    countdown_stop(cd);

    return edp_dev;

exit3:
    osl_free(edp_dev->send_buf);
exit2:
    osl_free(edp_dev->recv_buf);
exit1:
    osl_free(edp_dev);
exit:
    countdown_stop(cd);
    return NULL;
}

int32_t iot_edp_step(void *dev, uint32_t timeout_ms)
{
    handle_t          cd       = countdown_start(timeout_ms);
    struct iot_edp_t *edp_dev  = (struct iot_edp_t *)dev;
    uint8_t           msg_type = EDP_MSG_TYPE_NONE;
    int32_t           ret      = ERR_OK;

    ret = edp_parse_data(edp_dev, &msg_type, cd);

    if ((time_count() - edp_dev->last_send_time) > (edp_dev->life_time - 10))
        edp_send_ping(edp_dev);
    countdown_stop(cd);
    return ret;
}

int32_t iot_edp_push_rawdata(void *dev, const uint8_t *target, void *raw_data, uint32_t data_len, uint32_t timeout_ms)
{
    handle_t cd  = countdown_start(timeout_ms);
    int32_t  ret = ERR_OK;

    ret = edp_push_data((struct iot_edp_t *)dev, target, raw_data, data_len, cd);
    countdown_stop(cd);

    return ret;
}

int32_t iot_edp_notify(void *dev, struct iot_data_t *data, uint32_t timeout_ms)
{
    handle_t cd  = countdown_start(timeout_ms);
    int32_t  ret = ERR_OK;

    ret = edp_save_data((struct iot_edp_t *)dev, data, cd);
    countdown_stop(cd);
    return ret;
}

int32_t iot_edp_close(void *dev, uint32_t timeout_ms)
{
    struct iot_edp_t *edp_dev = (struct iot_edp_t *)dev;

    edp_logout(edp_dev);
    osl_free(edp_dev->recv_buf);
    osl_free(edp_dev->send_buf);
    osl_free(dev);

    return ERR_OK;
}
