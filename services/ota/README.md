# 远程升级OTA

[![Language](https://img.shields.io/badge/Language-C-brightgreen.svg?style=flat-square "Language")]()

- [概述](#概述)
- [应用流程](#应用流程)
- [用户接口说明](#用户接口说明)
- [用户用例](#用户用例)

## 概述

远程升级OTA服务可对终端的模组固件升级（FOTA）和MCU应用软件升级（SOTA）。两种方式使用了同一个数据通道，通过不同的参数来区分不同的升级内容。

## 应用流程

![远程升级流程](https://open.iot.10086.cn/doc/ota/images/OTA/%E5%8D%87%E7%BA%A7%E6%B5%81%E7%A8%8B.jpg)

## 用户接口说明

### 远程升级初始化

```c
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
int32_t ota_init(ota_evt_cb evt_cb, const uint8_t *dev_token, const uint8_t *dev_id, const uint8_t *version, const uint8_t *manuf, const uint8_t *model, uint8_t type, boolean cdn);
```

### 释放远程升级资源

```c
/**
 * @brief OTA初始化内存释放
 */
void ota_deinit(void);
```

### 上报版本号

对应应用流程步骤1。

```c
/**
 * @brief 上报版本号
 * @param timeout_ms 超时时间（毫秒）
 * @return 0-成功，<0-错误码，<100-平台响应错误码，>=100-HTTP状态码
 */
int32_t ota_post_version(uint32_t timeout_ms);
```

> *上报的版本号内容来自于初始化接口设定的版本信息。*

### 检测升级任务

对应应用流程步骤3。

```c
/**
 * @brief 检测升级任务
 * @param task_info 平台响应参数（需用户保存）
 * @param timeout_ms 超时时间（毫秒）
 * @return 0-成功，<0-错误码，<100-平台响应错误码，>=100-HTTP状态码
 */
int32_t ota_check_task(struct ota_task_info_t *task_info, uint32_t timeout_ms)
```

> *升级任务类型及各项参数均来自于初始化接口设定的信息。检查成功时，获得的具体任务信息通过`task_info`返回。*

### 检测升级token

对应应用流程步骤4。

```c
/**
 * @brief 检测升级token（token有效期为2天）
 * @param token token参数（通过检测升级任务获取）
 * @param timeout_ms 超时时间（毫秒）
 * @return 0-成功，<0-错误码，<100-平台响应错误码，>=100-HTTP状态码
 */
int32_t ota_check_token(uint8_t *token, uint32_t timeout_ms);
```

### 下载升级包

对应应用流程步骤5。

```c
/**
 * @brief 下载升级包（通过事件返回升级包数据）
 * @param token token参数（通过检测升级任务获取）
 * @param range_start 下载起始字节（从0开始）
 * @param range_size 下载分片大小（字节）
 * @param timeout_ms 超时时间（毫秒）
 * @return 0-成功，<0-错误码，<100-平台响应错误码，>=100-HTTP状态码
 */
int32_t ota_download_package(uint8_t *token, uint32_t range_start, uint32_t range_size, uint32_t timeout_ms);
```

> *如果设备侧使用了分片方式下载升级包，下载每个分片前，都需要调用**检查升级token接口**。*

### 上报下载进度或升级状态

对应应用流程步骤6。

```c
/**
 * @brief 上报下载进度与升级状态
 * @param token token参数（通过检测升级任务获取）
 * @param status_code <=100-下载进度，>100-状态码
 * @param timeout_ms 超时时间（毫秒）
 * @return 0-成功，<0-错误码，<100-平台响应错误码，>=100-HTTP状态码
 */
int32_t ota_report_status(uint8_t *token, uint32_t status_code, uint32_t timeout_ms);
```

## 用户用例

请参考远程OTA[示例代码](../../examples/ota)。