/**
 * Copyright (c), 2012~2021 iot.10086.cn All Rights Reserved
 * @file    func_ops.c
 * @date    2020/11/17
 * @brief   功能点操作接口，用于功能点值的设置及获取。接口列表由平台根
 *          据用户定义的产品功能点自动生成，用户需在接口内实现操作逻辑
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "err_def.h"
#include "plat_osl.h"
#include "log.h"
#include "ringa_tlv.h"
#include "func_ops.h"
#include "func_wrapper.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define SET_BIT_64(x, offset) (x | ((uint64_t)1 << (offset)))
#define CLEAR_BIT_64(x, offset) (x & ~((uint64_t)1 << (offset)))

#define FAULT_STATUS(x, y, z) (x = (z) ? SET_BIT_64(x, y) : CLEAR_BIT_64(x, y))

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
/****************************** Func List !!! ********************************/
static struct device_func_handle_t dev_func_list[] = {
    FUNC_HANDLE_BOTH(switch1, 1, BOOL) 
};
/***************************** Auto Generated !!! ****************************/

/******************************* Error Code !!! ******************************/
/***************************** Auto Generated !!! ****************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
int32_t func_set_val(uint16_t func_id, uint8_t *val, uint16_t val_len)
{
    uint16_t i = 0;
    
    for (i = 0; i < ARRAY_SIZE(dev_func_list); i++)
    {
        if (func_id == dev_func_list[i].info.func_id)
        {
            return dev_func_list[i].func_set(val, val_len);
        }
    }
    return ERR_INVALID_PARAM;
}

int32_t func_get_val(uint16_t func_id, uint8_t *buf, uint16_t buf_len, uint16_t func_lv)
{
    uint16_t query_len = 0;
    uint16_t offset = 0;
    uint16_t i = 0;

    for (i = 0; i < ARRAY_SIZE(dev_func_list); i++)
    {
        if (((0 == func_id) || (func_id == dev_func_list[i].info.func_id)) && dev_func_list[i].func_get)
        {
            uint32_t func_info = (func_lv << 16) | dev_func_list[i].info.func_id;

            /** 最短的数据（BOOL型）打包长度需要11字节*/
            if ((buf_len - query_len) < 11)
                return 0;

            /** 打包功能点信息*/
            query_len += ringa_tlv_ser(buf + query_len, RINGA_TLV_INT, (uint8_t *)&func_info, 4);

            /** 打包值*/
            buf[query_len++] = dev_func_list[i].info.data_type;
            if (0 == (offset = dev_func_list[i].func_get(buf + query_len + 2, buf_len - query_len - 2)))
                return 0;
            buf[query_len++] = offset & 0xFF;
            buf[query_len++] = (offset >> 8) & 0xFF;
            query_len += offset;
        }
    }
    return query_len;
}

static int32_t binary_to_int(uint8_t *val)
{
    int32_t value = 0;
    osl_memcpy(&value, val, sizeof(int32_t));
    return value;
}

static float64_t binary_to_float(uint8_t *val)
{
    float64_t value = 0;
    osl_memcpy(&value, val, sizeof(float64_t));
    return value;
}

static int32_t set_by_binary(void *value, uint8_t *val, uint16_t buf_len, uint16_t val_len)
{
    if (buf_len < val_len)
        return 0; 
    osl_memcpy(val, value, val_len);
    return val_len;
}

/******************************** Cmd API !!! ********************************/
int32_t func_switch1_set(uint8_t *val, uint16_t val_len)
{
    boolean value = (boolean)(*val);
    /** 根据变量value的值，填入下发控制逻辑 */
    log_info("bool value %d\n", value);

    /***/
    return 0;
}

/***************************** Auto Generated !!! ****************************/

/*********************** Get Upload And Query Value !!! **********************/
int32_t func_switch1_get(uint8_t *val, uint16_t buf_len)
{
    boolean value;          
    /** 请填入功能点值的获取逻辑，并将值赋给变量value */
    value = 1;
    
    /***/
    return set_by_binary(&value, val, buf_len, sizeof(boolean));
}

/***************************** Auto Generated !!! ****************************/

/******************************** Upload API !!! *****************************/
int32_t func_switch1_upload(uint32_t timeout_ms)
{
    return func_upload(1, 0, timeout_ms);
}

/***************************** Auto Generated !!! ****************************/
