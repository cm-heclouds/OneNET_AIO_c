/**
 * Copyright (c), 2012~2021 iot.10086.cn All Rights Reserved
 * @file    func_ops.h
 * @date    2020/11/17
 * @brief   功能点操作接口文件，接口列表由平台根据用户的功能点定义自动生成
 */

#ifndef __FUNC_OPS_H__
#define __FUNC_OPS_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "data_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*****************************************************************************/
/* External Definition ( Constant and Macro )                                */
/*****************************************************************************/
#define FUNC_LEVEL_UPLOAD     0x03
#define FUNC_LEVEL_QUERY      0x05
#define FUNC_LEVEL_EVENT      0x06
    
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define FUNC_INFO(y, z)             \
    {                               \
        y, TYPE_##z                 \
    }

/** 可上报可下发*/
#define FUNC_HANDLE_BOTH(x, y, z)                           \
    {                                                       \
        FUNC_INFO(y, z), func_##x##_set, func_##x##_get     \
    }

/** 只上报*/
#define FUNC_HANDLE_UP(x, y, z)                             \
    {                                                       \
        FUNC_INFO(y, z), NULL, func_##x##_get               \
    }

/** 只下发*/
#define FUNC_HANDLE_DOWN(x, y, z)                           \
    {                                                       \
        FUNC_INFO(y, z), func_##x##_set, NULL               \
    }

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/    
enum device_data_type_t
{
    /** 布尔型*/
    TYPE_BOOL = 1,
    /** 枚举型*/
    TYPE_ENUM,
    /** 数值型 - 整数*/
    TYPE_INT,
    /** 数值型 - 浮点数*/
    TYPE_FLOAT,
    /** 透传型*/
    TYPE_BINARY,
    /** 故障型*/
    TYPE_ERROR,
    /** 字符型*/
    TYPE_STRING,
    /** 事件型*/
    TYPE_EVENT,
    TYPE_DUMMY = 0x7FFFFFFF
};

struct device_func_info_t
{
    uint32_t func_id;
    enum device_data_type_t data_type;
};

typedef int32_t (*device_func_set)(uint8_t *val, uint16_t val_len);
typedef int32_t (*device_func_get)(uint8_t *val, uint16_t buf_len);

struct device_func_handle_t
{
    struct device_func_info_t info;
    device_func_set func_set;
    device_func_get func_get;
};

/***************************** Function Type !!! *****************************/
/***************************   Auto Generated !!! ****************************/

/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
int32_t func_set_val(uint16_t func_id, uint8_t *val, uint16_t val_len);

int32_t func_get_val(uint16_t func_id, uint8_t *buf, uint16_t buf_len, uint16_t func_lv);

/******************************** Cmd API !!! ********************************/
int32_t func_switch1_set(uint8_t *val, uint16_t val_len);

/***************************** Auto Generated !!! ****************************/

/*********************** Get Upload And Query Value !!! **********************/
int32_t func_switch1_get(uint8_t *val, uint16_t buf_len);

/***************************** Auto Generated !!! ****************************/

/******************************** Upload API !!! *****************************/
int32_t func_switch1_upload(uint32_t timeout_ms);

/***************************** Auto Generated !!! ****************************/


#ifdef __cplusplus
}
#endif

#endif

