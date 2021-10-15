/**
 * Copyright (c), 2012~2021 iot.10086.cn All Rights Reserved
 *
 * @file main.c
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include <stdio.h>

#include "func_ops.h"
#include "func_wrapper.h"
#include "ringa_api.h"
#include "ringa_reg.h"
#include "ringa_cmd.h"
#include "plat_osl.h"
#include "plat_time.h"
#include "err_def.h"
#include "log.h"
#ifdef CONFIG_GENERAL_OTA
#include "ota_api.h"
#endif

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define PARAM_FILE  "ringa_params"
#define PRODUCT_ID  ""
#define DEVICE_NAME ""

#ifdef CONFIG_GENERAL_OTA
#define DEVICE_VERSION "v1.0"
#define DEVICE_MANUF   "000"
#define DEVICE_MODEL   "00001"

#define OTA_API_TIMEOUT_MS 3000
#define OTA_FRAGMENT_SIZE  1024
#endif

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
struct device_t
{
    uint8_t dev_token[256];
    uint8_t dev_id[32];
    uint8_t reg_result;
#ifdef CONFIG_GENERAL_OTA
    uint8_t fota_flag;
#endif
};

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static struct device_t ringa_dev;

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
static int32_t get_login_token(int8_t *token)
{
    FILE *   fp        = NULL;
    uint32_t token_len = 0;

    fp = fopen(PARAM_FILE, "r");
    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        token_len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        fread(token, token_len, 1, fp);
        fclose(fp);
        return ERR_OK;
    }
    return ERR_INVALID_DATA;
}

static void set_login_token(int8_t *token)
{
    FILE *fp = NULL;

    fp = fopen(PARAM_FILE, "w");
    if (fp)
    {
        fwrite(token, osl_strlen(token), 1, fp);
        fclose(fp);
    }
}

#ifdef CONFIG_GENERAL_OTA
static int32_t ota_event_callback(enum ota_state_t state, uint8_t *data, uint32_t data_len)
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

static void device_ota(void)
{
    int32_t                ret         = 0;
    struct ota_task_info_t task_info   = {0};
    uint32_t               range_start = 0;
    
    ota_init(ota_event_callback, ringa_dev.dev_token, ringa_dev.dev_id, DEVICE_VERSION, DEVICE_MANUF, DEVICE_MODEL, 1,
             0);

    /** 检测升级任务 */
    if (0 != (ret = ota_check_task(&task_info, OTA_API_TIMEOUT_MS)))
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
    while (range_start < task_info.size - 1)
    {
        //每次下载前先校验token
        if (0 != (ret = ota_check_token(task_info.token, OTA_API_TIMEOUT_MS)))
        {
            log_info("ota check token fail, err = %d\n", ret);
            goto exit;
        }
        if (0 == (ret = ota_download_package(task_info.token, range_start, OTA_FRAGMENT_SIZE, OTA_API_TIMEOUT_MS)))
        {
            range_start += OTA_FRAGMENT_SIZE;
        }
        else
        {
            log_info("ota download package fail, start = %d, err = %d\n", range_start, ret);
            if (0 >= --task_info.retry)
            {
                log_info("ota download package fail, exit\n");
                goto exit;
            }
            time_delay(task_info.interval);
        }
    }
    log_info("ota download package ok\n");

    /** 上报下载成功 */
    if (0 != (ret = ota_report_status(task_info.token, OTA_DOWNLOAD_SUCCESS, OTA_API_TIMEOUT_MS)))
    {
        log_info("ota report \"download package ok\" fail, err = %d\n", ret);
        goto exit;
    }
    log_info("ota report \"download package ok\" ok\n");

    /** 上报升级成功 */
    if (0 != (ret = ota_report_status(task_info.token, OTA_UPGRADE_SUCCESS, OTA_API_TIMEOUT_MS)))
    {
        log_info("ota report \"upgrade ok\" fail, err = %d\n", ret);
        goto exit;
    }
    log_info("ota report \"upgrade ok\" ok\n");

exit:
    ota_deinit();
}
#endif

static int32_t dev_register_callback(enum ringa_reg_event_e reg_event, void *event_data, uint32_t event_data_len)
{
    log_info("Device register event : %d\n", reg_event);
    switch (reg_event)
    {
        case RINGA_REG_EVT_SUCCESSED:
        {
            struct ringa_reg_result_t *reg_result = (struct ringa_reg_result_t *)event_data;

            osl_strcpy(ringa_dev.dev_token, reg_result->dev_token);
            osl_strcpy(ringa_dev.dev_id, reg_result->dev_id);
            // set_login_token(ringa_dev.dev_token);
            ringa_dev.reg_result = 1;
            break;
        }
        default:
            break;
    }
    return ERR_OK;
}

static int32_t device_event_callback(struct device_event_t *evt)
{
    switch (evt->evt_id)
    {
        case DEVICE_EVT_ID_MODULE_RESET:
        {
            log_info("Module Reset\n");
            break;
        }
#ifdef CONFIG_GENERAL_OTA
        case DEVICE_EVT_ID_FOTA_REQUEST:
        {
            log_info("Start Upgrade\n");
            ringa_dev.fota_flag = 1;
            break;
        }
#endif
        default:
            break;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int32_t ret = ERR_OK;

    /** 通过smartconfig、softAP、蓝牙等手段从APP侧获取binding_token和connection_id
     *  如果设备使用直连或蜂窝网络上网，则可以直接使用产品ID和设备标识
     */
    uint8_t *binding_token = PRODUCT_ID;
    uint8_t *connection_id = DEVICE_NAME;

    /** 设备注册 */
    log_info("Device register start\n");
    ringa_register(PRODUCT_ID, DEVICE_NAME, connection_id, binding_token, dev_register_callback, 3000);
    if (1 != ringa_dev.reg_result)
    {
        log_error("Deivce register failed\n");
        return -1;
    }
    log_error("Deivce register successfully\n");

    /** 事件回调 */
    func_wrapper_init(device_event_callback);

    /** 设备登录 */
    ret = ringa_login(PRODUCT_ID, DEVICE_NAME, ringa_dev.dev_token, 90, 3000);
    if (ERR_OK != ret)
    {
        log_error("Device login failed, ret = %d\n", ret);
        return -1;
    }
    log_info("Device login successfully\n");

#ifdef CONFIG_GENERAL_OTA
    ota_init(ota_event_callback, ringa_dev.dev_token, ringa_dev.dev_id, DEVICE_VERSION, DEVICE_MANUF, DEVICE_MODEL, 1,
             0);
    ota_post_version(OTA_API_TIMEOUT_MS);
    ota_deinit();
#endif

    /** 数据上报 */
    func_switch1_upload(3000);

    while (1)
    {
        ringa_step(200);
#ifdef CONFIG_GENERAL_OTA
        if (ringa_dev.fota_flag)
        {
            break;
        }
#endif
    }

    ringa_logout(3000);
    log_info("Device logout\n");
#ifdef CONFIG_GENERAL_OTA
    if (ringa_dev.fota_flag)
    {
        device_ota();
    }
#endif

    return 0;
}
