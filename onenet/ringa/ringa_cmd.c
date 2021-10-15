/**
 * Copyright (c), 2012~2021 iot.10086.cn All Rights Reserved
 *
 * @file ringa_cmd.c
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "ringa_cmd.h"

#include "plat_osl.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/

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
int32_t ringa_cmd_pack(uint8_t *buf, uint32_t cmd, enum ringa_tlv_type_e type, uint8_t *cmd_data, uint16_t cmd_data_len)
{
    uint32_t offset = 0;

    offset += ringa_tlv_ser(buf, RINGA_TLV_INT, (uint8_t *)&cmd, sizeof(cmd));
    if (cmd_data)
    {
        offset += ringa_tlv_ser(buf + offset, type, cmd_data, cmd_data_len);
    }

    return offset;
}

int32_t ringa_cmd_get_data_len(uint8_t *buf, uint32_t *cmd, uint16_t *cmd_data_len)
{
    return ringa_tlv_get_next_len(buf + 7);
}

int32_t ringa_cmd_parse(uint8_t *buf, uint32_t *cmd, enum ringa_tlv_type_e *type, uint8_t **cmd_data,
                        uint16_t *cmd_data_len)
{
    uint32_t offset   = 0;
    uint8_t *cmd_ptr  = NULL;
    uint32_t cmd_type = 0;

    offset += ringa_tlv_deser(buf, NULL, &cmd_ptr, NULL);
    osl_memcpy(cmd, cmd_ptr, 4);
    cmd_type = *cmd >> 16;
    if (RINGA_CMD_TYPE_PROPERTY_WIRTE == cmd_type)
    {
        offset += ringa_tlv_deser(buf + offset, type, cmd_data, (uint16_t *)cmd_data_len);
    }

    return offset;
}
