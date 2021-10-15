/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file tm_user.c
 * @date 2020/05/14
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include <stdio.h>

#include "tm_data.h"
#include "tm_api.h"
#include "tm_user.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
/*************************** Property Func List ******************************/
struct tm_prop_tbl_t tm_prop_list[] = {
    TM_PROPERTY_RW(float32),
    TM_PROPERTY_RW(int32)
};
uint16_t tm_prop_list_size = ARRAY_SIZE(tm_prop_list);
/****************************** Auto Generated *******************************/


/***************************** Service Func List *******************************/
struct tm_svc_tbl_t tm_svc_list[] = {
    TM_SERVICE($SyncGetFile),
    TM_SERVICE(add)
};
uint16_t tm_svc_list_size = ARRAY_SIZE(tm_svc_list);
/****************************** Auto Generated *******************************/

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
/**************************** Property Func Read *****************************/
int32_t tm_prop_float32_rd_cb(void *data)
{
    float32_t val = 0;

    /** 根据业务逻辑获取功能点值，设置到val */
    val = 3.14;

    tm_data_struct_set_float(data, "float32", val);

    return 0;
}

int32_t tm_prop_int32_rd_cb(void *data)
{
    int32_t val = 0;

    /** 根据业务逻辑获取功能点值，设置到val */
    val = 66;

    tm_data_struct_set_int32(data, "int32", val);

    return 0;
}

/****************************** Auto Generated *******************************/


/**************************** Property Func Write ****************************/
int32_t tm_prop_float32_wr_cb(void *data)
{
    float32_t val = 0;

    tm_data_get_float(data, &val);

    /** 根据变量val的值，填入下发控制逻辑 */
    printf("%s: Write value %f\n", __FUNCTION__, val);

    return 0;
}

int32_t tm_prop_int32_wr_cb(void *data)
{
    int32_t val = 0;

    tm_data_get_int32(data, &val);

    /** 根据变量val的值，填入下发控制逻辑 */
    printf("%s: Write value %d\n", __FUNCTION__, val);

    return 0;
}

/****************************** Auto Generated *******************************/


/**************************** Service Func Invoke ****************************/
int32_t tm_file_callback(enum tm_file_event_e event, void *event_data)
{
    switch(event)
    {
        case TM_FILE_EVENT_POST_FAILED:
            printf("%s POST_FAILED, reason = [%s]\n", __FUNCTION__, (int8_t *)event_data);
            break;
        case TM_FILE_EVENT_POST_SUCCESSED:
            printf("%s POST SUCCESSED, file ID = [%s]\n", __FUNCTION__, (int8_t *)event_data);
            break;
        case TM_FILE_EVENT_GET_DATA:
            printf("%s GET DATA [seq: %d, len: %d]\n", __FUNCTION__, ((struct tm_file_data_t *)event_data)->seq, ((struct tm_file_data_t *)event_data)->data_len);
            break;
        default:
                break;
    }

    return 0;
}

int32_t tm_svc_$SyncGetFile_cb(void *in_data, void *out_data)
{
    struct svc_$SyncGetFile_in_t in_param;
    struct svc_$SyncGetFile_out_t out_param;

    int32_t ret = 0;

    tm_data_struct_get_string(in_data, "fileID", &in_param.fileID);
    tm_data_struct_get_string(in_data, "fileName", &in_param.fileName);
    tm_data_struct_get_int32(in_data, "size", &in_param.size);

    /** 根据输入参数，生成输出参数 */
    tm_file_get(tm_file_callback, in_param.fileID, in_param.size, 800, 2000);
    out_param.code = 200;
    out_param.message = "OK";

    tm_data_struct_set_enum(out_data, "code", out_param.code);
    tm_data_struct_set_string(out_data, "message", out_param.message);

    return ret;
}

int32_t tm_svc_add_cb(void *in_data, void *out_data)
{
    struct svc_add_in_t in_param;
    struct svc_add_out_t out_param;

    int32_t ret = 0;

    tm_data_struct_get_int32(in_data, "a", &in_param.a);
    tm_data_struct_get_int32(in_data, "b", &in_param.b);

    /** 根据输入参数，生成输出参数 */
    out_param.c = in_param.a + in_param.b;

    tm_data_struct_set_int32(out_data, "c", out_param.c);

    return ret;
}

/****************************** Auto Generated *******************************/


/**************************** Property Func Notify ***************************/
int32_t tm_prop_float32_notify(void *data, float32_t val, uint64_t timestamp, uint32_t timeout_ms)
{
    void *resource = NULL;
    int32_t ret = 0;

    if(NULL == data)
    {
        resource = tm_data_create();
    }
    else
    {
        resource = data;
    }

    tm_data_set_float(resource, "float32", val, timestamp);

    if(NULL == data)
    {
        ret = tm_post_property(resource, timeout_ms);
    }

    return ret;
}

int32_t tm_prop_int32_notify(void *data, int32_t val, uint64_t timestamp, uint32_t timeout_ms)
{
    void *resource = NULL;
    int32_t ret = 0;

    if(NULL == data)
    {
        resource = tm_data_create();
    }
    else
    {
        resource = data;
    }

    tm_data_set_int32(resource, "int32", val, timestamp);

    if(NULL == data)
    {
        ret = tm_post_property(resource, timeout_ms);
    }

    return ret;
}

/****************************** Auto Generated *******************************/


/***************************** Event Func Notify *****************************/
int32_t tm_event_alert_notify(void *data, struct event_alert_t val, uint64_t timestamp, uint32_t timeout_ms)
{
    void *resource = NULL;
    void *structure = tm_data_struct_create();
    int32_t ret = 0;

    if(NULL == data)
    {
        resource = tm_data_create();
    }
    else
    {
        resource = data;
    }

    tm_data_struct_set_int32(structure, "cnt", val.cnt);

    tm_data_set_struct(resource, "alert", structure, timestamp);

    if(NULL == data)
    {
        ret = tm_post_event(resource, timeout_ms);
    }

    return ret;
}

/****************************** Auto Generated *******************************/

