/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file tm_user.h
 * @date 2020/05/14
 * @brief
 */

#ifndef __TM_USER_H__
#define __TM_USER_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "data_types.h"
#include "tm_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*****************************************************************************/
/* External Definition ( Constant and Macro )                                */
/*****************************************************************************/

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/
/****************************** Structure type *******************************/
struct event_alert_t
{
    int32_t cnt;
};


struct svc_$SyncGetFile_in_t
{
    int8_t *fileID;
    int8_t *fileName;
    int32_t size;
};
struct svc_$SyncGetFile_out_t
{
    int32_t code;
    int8_t *message;
};
struct svc_add_in_t
{
    int32_t a;
    int32_t b;
};
struct svc_add_out_t
{
    int32_t c;
};
/****************************** Auto Generated *******************************/

/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
/*************************** Property Func List ******************************/
extern struct tm_prop_tbl_t tm_prop_list[];
extern uint16_t tm_prop_list_size;
/****************************** Auto Generated *******************************/


/**************************** Service Func List ******************************/
extern struct tm_svc_tbl_t tm_svc_list[];
extern uint16_t tm_svc_list_size;
/****************************** Auto Generated *******************************/


/**************************** Property Func Read ****************************/
int32_t tm_prop_float32_rd_cb(void *data);
int32_t tm_prop_int32_rd_cb(void *data);
/****************************** Auto Generated *******************************/


/**************************** Property Func Write ****************************/
int32_t tm_prop_float32_wr_cb(void *data);
int32_t tm_prop_int32_wr_cb(void *data);
/****************************** Auto Generated *******************************/


/**************************** Service Func Invoke ****************************/
int32_t tm_file_callback(enum tm_file_event_e event, void *event_data);
int32_t tm_svc_$SyncGetFile_cb(void *in_data, void *out_data);
int32_t tm_svc_add_cb(void *in_data, void *out_data);
/****************************** Auto Generated *******************************/


/**************************** Property Func Notify ***************************/
int32_t tm_prop_float32_notify(void *data, float32_t val, uint64_t timestamp, uint32_t timeout_ms);
int32_t tm_prop_int32_notify(void *data, int32_t val, uint64_t timestamp, uint32_t timeout_ms);
/****************************** Auto Generated *******************************/


/***************************** Event Func Notify *****************************/
int32_t tm_event_alert_notify(void *data, struct event_alert_t val, uint64_t timestamp, uint32_t timeout_ms);
/****************************** Auto Generated *******************************/

#ifdef __cplusplus
}
#endif

#endif
