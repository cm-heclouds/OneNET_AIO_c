/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file main.c
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include <stdio.h>

#include "data_types.h"
#include "plat_time.h"
#include "tm_api.h"
#include "tm_user.h"
#include "tm_data.h"
#include "log.h"
#ifdef CONFIG_GENERAL_OTA
#include "ota_api.h"
#endif

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define PRODUCT_ID  ""
#define DEVICE_NAME ""
#define ACCESS_KEY  ""

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
#ifdef CONFIG_GENERAL_OTA
static uint8_t start_ota = 1;
#endif

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
#if 0 //多功能点同时上报
// 例如三个分别名为a,b,c的功能点，功能点数据类型分别为bool，int32，flaat32

static int32_t tm_prop_combine_notify(uint64_t timeout_ms)
{
    void *data = tm_data_create();

    tm_prop_a_notify(data, TRUE, 0, 0);     //自动生成，位于tm_user.h
    tm_prop_b_notify(data, 3, 0, 0);        //自动生成，位于tm_user.h  
    tm_prop_c_notify(data, 3.14, 0, 0);     //自动生成，位于tm_user.h

    return tm_post_property(data, timeout_ms);
}
#endif

#if 0 // LBS系统功能点
static int32_t tm_prop_lbs_notify(uint64_t timeout_ms)
{
    void *data = tm_data_create();
    struct prop_element_t val;

    val.temp = 555;
    val.mode = 0x01;
    val.name = "demon soul";
    tm_prop_element_notify(data, val, 1591926170000, 0);

    val.temp = 666;
    val.mode = 0x02;
    val.name = "darkness soul";
    tm_prop_element_notify(data, val, 0, 0);

    val.temp = 777;
    val.mode = 0x03;
    val.name = "bloodborn";
    tm_prop_element_notify(data, val, 0, 0);

    return tm_post_property(data, timeout_ms);
}
#endif

#ifdef CONFIG_GENERAL_OTA
static int32_t ota_event_callback(enum ota_state_t state, uint8_t *data, uint32_t data_len)
{
    data[data_len] = '\0';
    switch (state)
    {
        case OTA_EVT_DOWNLOAD_PACKAGE:
        {
            log_info("download len = %d\n", data_len);
            break;
        }
        case OTA_EVT_CHECK_TASK:
            log_info("Check task:\n%s\n", data);
            break;
        case OTA_EVT_CHECK_TOKEN:
            log_info("Check token:\n%s\n", data);
            break;
        case OTA_EVT_REPORT_STATUS:
            log_info("Report status:\n%s\n", data);
            break;
        default:
            break;
    }
    return 0;
}

static int32_t ota_start_callback(void)
{
    log_info("Start OTA\n");
    start_ota = 1;
    return 0;
}
#endif

int main(int arg, char **argv[])
{
    struct tm_downlink_tbl_t dl_tbl;
#ifdef CONFIG_GENERAL_OTA
    struct ota_task_info_t task_info       = {0};
    int32_t                i               = 0;
    uint8_t *              current_version = "v1.0";
#endif

    dl_tbl.prop_tbl      = tm_prop_list;
    dl_tbl.prop_tbl_size = tm_prop_list_size;
    dl_tbl.svc_tbl       = tm_svc_list;
    dl_tbl.svc_tbl_size  = tm_svc_list_size;
#ifdef CONFIG_GENERAL_OTA
    dl_tbl.ota_cb = ota_start_callback;

    /** 上报当前版本*/
    ota_init(ota_event_callback, PRODUCT_ID, DEVICE_NAME, ACCESS_KEY, current_version, 1);
    ota_post_version(3000);
#endif

    /** 设备初始化*/
    if (0 != tm_init(&dl_tbl))
    {
        goto exit;
    }
    printf("ThingModel init ok\n");

    /** 设备登录*/
    if (0 != tm_login((const uint8_t *)PRODUCT_ID, (const uint8_t *)DEVICE_NAME, (const uint8_t *)ACCESS_KEY, 3000))
    {
        goto exit;
    }
    printf("ThingModel login ok\n");

    tm_prop_int32_notify(NULL, 27, 0, 3000);

#ifdef CONFIG_GENERAL_OTA
    while (1 != start_ota)
#else
    while (1)
#endif
    {
        /** 数据解析*/
        if (0 != tm_step(200))
        {
            break;
        }
        time_delay_ms(50);
    }

    /** 设备注销*/
    tm_logout(3000);

#ifdef CONFIG_GENERAL_OTA
    ota_check_task(&task_info, 3000);
    log_info("TaskInfo ==>\n");
    log_info("target    : %s\n", task_info.target);
    log_info("type      : %s\n", (1 == task_info.type) ? "Whole" : "Diff");
    log_info("size      : %d\n", task_info.size);
    log_info("md5       : %s\n", task_info.md5);

    /** 以1024字节的分片下载升级包，升级包数据通过ota_event_callback返回*/
    for (i = 0; i < task_info.size;)
    {
        /** 每次下载分片前检查任务状态，防止任务过期*/
        ota_check_token(task_info.tid, 3000);
        ota_download_package(task_info.tid, i, ((task_info.size - i) < 1024) ? (task_info.size - i) : 1024, 3000);
        i += 1024;
        /** 可选上报下载进度*/
    }
    /** 校验md5，应用升级包*/

    /** 应用升级包成功后，上报“升级成功”状态*/
    ota_report_status(task_info.tid, 201, 3000);
    ota_deinit();
#endif
exit:
    return 0;
}
