/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file ota_api.h
 * @brief
 */

#ifndef __OTA_API_H__
#define __OTA_API_H__

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
/** 升级错误码 */
#define OTA_ERR_OK                     0  //成功
#define OTA_CHECK_NO_DEVICE            1  //设备不存在
#define OTA_CHECK_PARAMS_ERROR         2  //请求参数错误
#define OTA_CHECK_TOKEN_DISCORD        4  // token和did不一致
#define OTA_CHECK_INTERNAL_ERROR       7  //平台内部错误
#define OTA_CHECK_MANUL_MODEL_DISCORD  8  //厂商或模组型号不一致
#define OTA_CHECK_VERSION_DISCORD      9  //版本号不一致
#define OTA_CHECK_NO_TASK              11 //没有升级任务
#define OTA_CHECK_TOKEN_EXPIRED        12 // token过期
#define OTA_CHECK_INVALID_TASK_STATE   17 //无效的任务状态
#define OTA_CHECK_TASK_TERMINATE       20 //升级任务结束
#define OTA_CHECK_INVALID_OPERATION    21 //无效的操作
#define OTA_CHECK_TASK_CANCELED        22 //升级任务被取消
#define OTA_CHECK_VERSION_FORMAT_ERROR 27 //版本格式错误
#define OTA_DOWNLOAD_NO_SOURCE         1  //未找到资源
#define OTA_DOWNLOAD_FILE_SIZE_DISCORD 2  //文件大小不一致
#define OTA_DOWNLOAD_INVALID_TOKEN     3  //无效的token

/** 升级状态码 */
#define OTA_DOWNLOAD_SUCCESS         101 //升级包下载成功
#define OTA_DOWNLOAD_NO_MEMORY       102 //下载失败，空间不足
#define OTA_DOWNLOAD_OUT_OF_MEMORY   103 //下载失败，内存溢出
#define OTA_DOWNLOAD_REQUEST_TIMEOUT 104 //下载请求超时
#define OTA_DOWNLOAD_LOW_POWER       105 //下载失败，电量不足
#define OTA_DOWNLOAD_BAD_SIGNAL      106 //下载失败，信号不良
#define OTA_DOWNLOAD_UNKNOWN_ERROR   107 //下载失败，未知异常
#define OTA_UPGRADE_SUCCESS          201 //升级成功
#define OTA_UPGRADE_LOW_POWER        202 //升级失败，电量不足
#define OTA_UPGRADE_OUT_OF_MEMORY    203 //升级失败，内存溢出
#define OTA_UPGRADE_VERSION_DISCORD  204 //升级失败，版本不一致
#define OTA_UPGRADE_MD5_ERROR        205 //升级失败，MD5校验失败
#define OTA_UPGRADE_UNKNOWN_ERROR    206 //升级失败，未知异常
#define OTA_UPGRADE_OVER_RETRY       207 //升级失败，超过重试次数
#define OTA_UPGRADE_EXPIRED          208 //升级过期

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/
struct ota_task_info_t
{
    uint8_t  target[32];
    uint8_t  md5[36];
    uint32_t size;
#ifdef CONFIG_TM
    uint32_t tid;
#else
    uint8_t  token[32];
    uint8_t  ip[32];
    uint32_t interval;
    int16_t  retry;
    uint8_t  signal;
    uint8_t  power;
#endif
    uint8_t type;
};

enum ota_state_t
{
    OTA_EVT_POST_VERSION = 0,
    OTA_EVT_CHECK_TASK,
    OTA_EVT_CHECK_TOKEN,
    OTA_EVT_DOWNLOAD_PACKAGE,
    OTA_EVT_REPORT_STATUS
};

typedef int32_t (*ota_evt_cb)(enum ota_state_t state, uint8_t *data, uint32_t data_len);

/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/
/**
 * @brief OTA初始化，需用户传入相关参数
 *
 * @param evt_cb 事件回调，用于接收平台响应数据
 * @param dev_token 鉴权信息
 * @param dev_id 设备id
 * @param version 当前版本号
 * @param manuf 厂商编号
 * @param model 模组编号
 * @param type 升级类型（1：fota，2：sota）
 * @param cdn 是否返回拉取升级包ip
 * @return 0-成功
 */
#ifndef CONFIG_TM
int32_t ota_init(ota_evt_cb evt_cb, const uint8_t *dev_token, const uint8_t *dev_id, const uint8_t *version,
                 const uint8_t *manuf, const uint8_t *model, uint8_t type, boolean cdn);
#else
int32_t ota_init(ota_evt_cb evt_cb, const uint8_t *product_id, const uint8_t *dev_name, const uint8_t *access_key,
                 const uint8_t *version, uint8_t type);
#endif

/**
 * @brief OTA初始化内存释放
 */
void ota_deinit(void);

/**
 * @brief 上报版本号
 * @param timeout_ms 超时时间（毫秒）
 * @return 0-成功，<0-错误码，<100-平台响应错误码，>=100-HTTP状态码
 */
int32_t ota_post_version(uint32_t timeout_ms);

/**
 * @brief 检测升级任务
 * @param task_info 平台响应参数（需用户保存）
 * @param timeout_ms 超时时间（毫秒）
 * @return 0-成功，<0-错误码，<100-平台响应错误码，>=100-HTTP状态码
 */
int32_t ota_check_task(struct ota_task_info_t *task_info, uint32_t timeout_ms);

/**
 * @brief 检测升级token（token有效期为2天）
 * @param token token参数（通过检测升级任务获取）
 * @param timeout_ms 超时时间（毫秒）
 * @return 0-成功，<0-错误码，<100-平台响应错误码，>=100-HTTP状态码
 */
#ifndef CONFIG_TM
int32_t ota_check_token(uint8_t *token, uint32_t timeout_ms);
#else
int32_t ota_check_token(uint32_t tid, uint32_t timeout_ms);
#endif

/**
 * @brief 下载升级包（通过事件返回升级包数据）
 * @param token token参数（通过检测升级任务获取）
 * @param range_start 下载起始字节（从0开始）
 * @param range_size 下载分片大小（字节）
 * @param timeout_ms 超时时间（毫秒）
 * @return 0-成功，<0-错误码，<100-平台响应错误码，>=100-HTTP状态码
 */
#ifndef CONFIG_TM
int32_t ota_download_package(uint8_t *token, uint32_t range_start, uint32_t range_size, uint32_t timeout_ms);
#else
int32_t ota_download_package(uint32_t tid, uint32_t range_start, uint32_t range_size, uint32_t timeout_ms);
#endif

/**
 * @brief 上报下载进度与升级状态
 * @param token token参数（通过检测升级任务获取）
 * @param status_code <=100-下载进度，>100-状态码
 * @param timeout_ms 超时时间（毫秒）
 * @return 0-成功，<0-错误码，<100-平台响应错误码，>=100-HTTP状态码
 */
#ifndef CONFIG_TM
int32_t ota_report_status(uint8_t *token, uint32_t status_code, uint32_t timeout_ms);
#else
int32_t ota_report_status(uint32_t tid, uint32_t status_code, uint32_t timeout_ms);
#endif

#ifdef __cplusplus
}
#endif

#endif
