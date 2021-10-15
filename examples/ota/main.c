/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file main.c
 * @brief ota demo
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "dev_token.h"
#include "ota_api.h"
#include "err_def.h"
#include "log.h"

#include <unistd.h>
/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define PRODUCT_ID "xxxxxx"
#define ACCESS_KEY "xxxxxx"
#define DEV_ID     "xxxxxx"
#define MANUF      "000"
#define MODEL      "00001"
#define VERSION    "1.0.0"

#define RANGE_SIZE 4096
#define TIMEOUT_MS 3000

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
int32_t ota_event_callback(enum ota_state_t state, uint8_t *data, uint32_t data_len)
{
    switch (state)
    {
        case OTA_EVT_DOWNLOAD_PACKAGE:
        {
            log_info("download len = %d\n", data_len);
            break;
        }
        default:
            break;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int32_t ret = ERR_NO_MEM;

    /** OTA初始化 */
    uint8_t dev_token[256] = {0};
    dev_token_generate(dev_token, SIG_METHOD_SHA1, 2524579200, PRODUCT_ID, NULL, ACCESS_KEY);
    if (0 != (ret = ota_init(ota_event_callback, dev_token, DEV_ID, VERSION, MANUF, MODEL, 1, true)))
    {
        log_info("ota init fail\n");
        return ret;
    }
    log_info("ota init ok\n");

    /** 上报当前版本号 */
    if (0 != (ret = ota_post_version(TIMEOUT_MS)))
    {
        log_info("ota post version fail, err = %d\n", ret);
        goto exit;
    }
    log_info("ota post version ok\n");

    /** 检测升级任务 */
    struct ota_task_info_t task_info;
    if (0 != (ret = ota_check_task(&task_info, TIMEOUT_MS)))
    {
        log_info("ota check task fail, err = %d\n", ret);
        goto exit;
    }
    log_info("ota check task ok\n");
    log_info("task_info.target = %s\n", task_info.target);
    log_info("task_info.token = %s\n", task_info.token);
    log_info("task_info.md5 = %s\n", task_info.md5);
    log_info("task_info.ip = %s\n", task_info.ip);
    log_info("task_info.size = %d\n", task_info.size);
    log_info("task_info.interval = %d\n", task_info.interval);
    log_info("task_info.signal = %d\n", task_info.signal);
    log_info("task_info.retry = %d\n", task_info.retry);
    log_info("task_info.power = %d\n", task_info.power);
    log_info("task_info.type = %d\n", task_info.type);

    /** 下载升级包 */
    uint32_t range_start = 0;
    while (range_start < task_info.size - 1)
    {
        //每次下载前先校验token
        if (0 != (ret = ota_check_token(task_info.token, TIMEOUT_MS)))
        {
            log_info("ota check token fail, err = %d\n", ret);
            goto exit;
        }
        if (0 == (ret = ota_download_package(task_info.token, range_start, RANGE_SIZE, TIMEOUT_MS)))
        {
            range_start += RANGE_SIZE;
        }
        else
        {
            log_info("ota download package fail, start = %d, err = %d\n", range_start, ret);
            if (0 >= --task_info.retry)
            {
                log_info("ota download package fail, exit\n");
                goto exit;
            }
            sleep(task_info.interval);
        }
    }
    log_info("ota download package ok\n");

    /** 上报下载成功 */
    if (0 != (ret = ota_report_status(task_info.token, OTA_DOWNLOAD_SUCCESS, TIMEOUT_MS)))
    {
        log_info("ota report \"download package ok\" fail, err = %d\n", ret);
        goto exit;
    }
    log_info("ota report \"download package ok\" ok\n");

    /** 上报升级成功 */
    if (0 != (ret = ota_report_status(task_info.token, OTA_UPGRADE_SUCCESS, TIMEOUT_MS)))
    {
        log_info("ota report \"upgrade ok\" fail, err = %d\n", ret);
        goto exit;
    }
    log_info("ota report \"upgrade ok\" ok\n");

exit:
    ota_deinit();
    return ret;
}
