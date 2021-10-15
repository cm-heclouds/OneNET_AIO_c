/**
 * Copyright (c), 2012~2021 iot.10086.cn All Rights Reserved
 *
 * @file ringa_tlv.h
 * @brief
 */

#ifndef __RINGA_TLV_H__
#define __RINGA_TLV_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "data_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/* External Definition ( Constant and Macro )                                */
/*****************************************************************************/

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/
enum ringa_tlv_type_e
{
    RINGA_TLV_BOOLEAN = 1,
    RINGA_TLV_ENUM,
    RINGA_TLV_INT,
    RINGA_TLV_FLOAT,
    RINGA_TLV_BINARY,
    RINGA_TLV_ERROR,
    RINGA_TLV_STRING
};

/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
uint16_t ringa_tlv_ser(uint8_t *buf, enum ringa_tlv_type_e type, uint8_t *val, uint16_t val_len);
uint16_t ringa_tlv_deser(uint8_t *buf, enum ringa_tlv_type_e *type, uint8_t **val, uint16_t *val_len);
uint16_t ringa_tlv_get_next_len(uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif
