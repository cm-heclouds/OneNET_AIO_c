/**
 * Copyright (c), 2012~2021 iot.10086.cn All Rights Reserved
 *
 * @file ringa_rcm.c
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "plat_osl.h"
#include "err_def.h"
#include "log.h"

#include "ringa_rcm.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define RCM_FRAME_SYNC 0x5A5A

#define RCM_FRAME_FIXED_FIELDS_LEN 6
#define RCM_FRAME_CHECKSUM_LEN     1

#define RCM_FRAME_SYNC_OFFSET 0
#define RCM_FRAME_LEN_OFFSET  2
#define RCM_FRAME_CMD_OFFSET  4
#define RCM_FRAME_DATA_OFFSET 5

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
#pragma pack(1)
struct rcm_frame_header_t
{
    uint16_t sync;
    uint16_t len;
    uint8_t  type    : 7;
    uint8_t  is_resp : 1;
};
#pragma pack()

struct rcm_parser_t
{
    struct rcm_transceiver_t transceiver;
    uint8_t *                buf;
    uint32_t                 buf_len;
    uint16_t                 invalid_data_len;
    uint16_t                 unparsed_data_len;
};
/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static struct rcm_parser_t rcm_parser;

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
static uint8_t calc_checksum8(uint8_t *frame, uint32_t frame_len)
{
    uint32_t checksum = 0;
    uint32_t i        = 0;

    for (i = 0; i < frame_len; i++)
    {
        checksum += frame[i];
    }

    return (uint8_t)(checksum & 0xFF);
}

int32_t rcm_init(struct rcm_transceiver_t *transceiver, uint32_t parse_buf_len)
{
    if ((NULL == transceiver) || (NULL == transceiver->send) || (NULL == transceiver->recv) || (0 >= parse_buf_len))
    {
        return ERR_INVALID_PARAM;
    }
    if (rcm_parser.buf)
    {
        osl_free(rcm_parser.buf);
    }

    rcm_parser.buf = osl_malloc(parse_buf_len);
    if (NULL == rcm_parser.buf)
    {
        return ERR_NO_MEM;
    }
    osl_memset(rcm_parser.buf, 0, parse_buf_len);
    rcm_parser.buf_len          = parse_buf_len;
    rcm_parser.transceiver.send = transceiver->send;
    rcm_parser.transceiver.recv = transceiver->recv;

    return ERR_OK;
}

uint8_t *rcm_frame_buf_malloc(uint16_t payload_len)
{
    uint32_t frame_buf_len = payload_len + RCM_FRAME_FIXED_FIELDS_LEN;
    void *   frame_buf     = osl_malloc(frame_buf_len);

    if (NULL == frame_buf)
    {
        return frame_buf;
    }

    osl_memset(frame_buf, 0, frame_buf_len);

    return ((uint8_t *)frame_buf + sizeof(struct rcm_frame_header_t));
}

void rcm_frame_buf_free(uint8_t *buf)
{
    if (buf)
    {
        osl_free(buf - sizeof(struct rcm_frame_header_t));
    }
}

static uint8_t get_response_flag(uint8_t *frame)
{
    return (frame[RCM_FRAME_CMD_OFFSET] >> 7);
}

static uint8_t get_frame_type(uint8_t *frame)
{
    return (frame[RCM_FRAME_CMD_OFFSET] & 0x7F);
}

static uint16_t get_frame_len(uint8_t *frame)
{
    return (frame[RCM_FRAME_LEN_OFFSET + 1] << 8) | (frame[RCM_FRAME_LEN_OFFSET]);
}

static uint8_t get_frame_checksum(uint8_t *frame, uint16_t frame_len)
{
    return (frame[frame_len - 1]);
}

enum rcm_msg_type_t rcm_frame_parse(uint8_t *is_resp, uint8_t **payload, uint16_t *payload_len)
{
    enum rcm_msg_type_t frame_type = RCM_MSG_TYPE_NONE;
    uint16_t            offset     = 0;
    uint16_t            frame_len  = 0;
    uint8_t             checksum   = 0;

    if (rcm_parser.invalid_data_len)
    {
        log_debug("Unparsed data %d, remove invalid data %d\n", rcm_parser.unparsed_data_len,
                  rcm_parser.invalid_data_len);
        rcm_parser.unparsed_data_len -= rcm_parser.invalid_data_len;
        osl_memcpy(rcm_parser.buf, rcm_parser.buf + rcm_parser.invalid_data_len, rcm_parser.unparsed_data_len);
        rcm_parser.invalid_data_len = 0;
    }

    rcm_parser.unparsed_data_len += rcm_parser.transceiver.recv(rcm_parser.buf + rcm_parser.unparsed_data_len,
                                                                rcm_parser.buf_len - rcm_parser.unparsed_data_len);

    for (offset = 0; offset < rcm_parser.unparsed_data_len; offset++)
    {
        if (rcm_parser.buf[offset] == 0x5A)
        {
            log_debug("Found head %d\n", offset);
            if (offset)
            {
                rcm_parser.unparsed_data_len -= offset;
                osl_memcpy(rcm_parser.buf, rcm_parser.buf + offset, rcm_parser.unparsed_data_len);
            }
            break;
        }
        else if (offset == (rcm_parser.unparsed_data_len - 1))
            rcm_parser.unparsed_data_len = 0;
    }

    if (rcm_parser.unparsed_data_len >= RCM_FRAME_FIXED_FIELDS_LEN)
    {
        if ((rcm_parser.buf[RCM_FRAME_SYNC_OFFSET] == 0x5A) && (rcm_parser.buf[RCM_FRAME_SYNC_OFFSET + 1] == 0x5A))
        {
            if (rcm_parser.buf_len >= (frame_len = get_frame_len(rcm_parser.buf)))
            {
                if (frame_len > rcm_parser.unparsed_data_len)
                    return frame_type;
                checksum = get_frame_checksum(rcm_parser.buf, frame_len);
                if (checksum == calc_checksum8(rcm_parser.buf, frame_len - 1))
                {
                    frame_type   = (enum rcm_msg_type_t)get_frame_type(rcm_parser.buf);
                    *payload     = rcm_parser.buf + sizeof(struct rcm_frame_header_t);
                    *payload_len = frame_len - RCM_FRAME_FIXED_FIELDS_LEN;
                    *is_resp     = get_response_flag(rcm_parser.buf);

                    rcm_parser.invalid_data_len = frame_len;
                    log_debug("Parsed %d\n", frame_type);
                    return frame_type;
                }
            }
        }
        rcm_parser.invalid_data_len = 1;
    }

    return frame_type;
}

uint32_t rcm_frame_pack_and_send(enum rcm_msg_type_t type, uint8_t is_resp, uint8_t *payload, uint16_t payload_len)
{
    uint8_t *                  send_buf     = NULL;
    uint8_t *                  checksum_ptr = NULL;
    struct rcm_frame_header_t *header       = NULL;

    if (0 == payload_len)
    {
        send_buf = osl_malloc(RCM_FRAME_FIXED_FIELDS_LEN);
        if (NULL == send_buf)
        {
            return ERR_NO_MEM;
        }
    }
    else
    {
        send_buf = payload - sizeof(struct rcm_frame_header_t);
    }
    checksum_ptr = send_buf + RCM_FRAME_FIXED_FIELDS_LEN + payload_len - RCM_FRAME_CHECKSUM_LEN;

    header          = (struct rcm_frame_header_t *)send_buf;
    header->sync    = RCM_FRAME_SYNC;
    header->len     = RCM_FRAME_FIXED_FIELDS_LEN + payload_len;
    header->is_resp = is_resp;
    header->type    = type;
    *checksum_ptr   = calc_checksum8(send_buf, header->len - RCM_FRAME_CHECKSUM_LEN);

    rcm_parser.transceiver.send(send_buf, header->len);
    if (0 == payload_len)
    {
        osl_free(send_buf);
    }

    return ERR_OK;
}
