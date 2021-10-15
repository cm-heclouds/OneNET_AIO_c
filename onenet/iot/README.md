# OneNET AIO 通用PaaS协议接入

[![Language](https://img.shields.io/badge/Language-C-brightgreen.svg?style=flat-square "Language")]()


- [概述](#概述)
- [用户接口说明](#用户接口说明)
- [用户用例](#用户用例)

  - [edp接入](#edp接入)
  - [mqtt接入](#mqtt接入)
  - [mqtt物联网套件接入](#mqtt物联网套件接入)
  - [nb-iot物联网套件接入](#nb-iot物联网套件接入)

## 概述

OneNET AIO通用PaaS协议接入目前支持四种方式：**EDP**、**MQTT**、**MQTT物联网套件**、**NB-IoT物联网套件**。这四种接入方式均遵循OneNET平台资源模型（**产品-设备-数据流-数据点**），可以封装为统一的操作接口。

## 用户接口说明

- 头文件

  ```c
  onenet/iot/iot_api.h
  ```

- 接口说明

  - 关键定义

    - 服务器连接参数配置

      ```c
      union network_cert_u
      {
          /** TLS证书缓冲区地址，CA证书形式*/
          const uint8_t *ca_cert;
          /** DTLS鉴权用PSK缓冲区地址*/
          const uint8_t *psk;
      };
      
      struct network_param_t
      {
          const uint8_t *      remote_addr;
          uint16_t             remote_port;
          uint16_t             cert_len;
          union network_cert_u cert_buf;
      };
      
      struct iot_cloud_config_t
      {
          /** 服务器网络连接参数，设置为空时使用协议内置的默认服务器配置*/
          struct network_param_t *server_param;
          /** OneNET平台创建的产品ID*/
          uint8_t *               product_id;
          /** 设备唯一识别码，产品内唯一。建议使用IMEI*/
          uint8_t *               device_sn;
          /** 认证信息*/
          uint8_t *               auth_code;
          /** 设备保活时间, 单位为s*/
          uint32_t                life_time;
      };
      ```
      
    - 统一数据结构

      ```c
      enum iot_data_type_e
      {
          IOT_DATA_TYPE_INT = 0, /**<整型数据，长度8字节 */
          IOT_DATA_TYPE_FLOAT,   /**<单精度浮点型数据 */
          IOT_DATA_TYPE_STRING,  /**<字符串型数据 */
          IOT_DATA_TYPE_BINARY,  /**<二进制数据 */
          IOT_DATA_TYPE_DUMMY = 0xFFFFFFFF
      };
      
      union iot_data_buf_u
      {
          /** 数据为int型时使用*/
          int64_t   val_int;
          /** 数据为float型时使用*/
          float64_t val_float;
          /** 数据为string型时使用*/
          uint8_t * val_str;
          /** 数据为binary型时使用*/
          struct
          {
              uint32_t buf_len;
              uint8_t *buf;
          } val_bin;
      };
      
      struct iot_data_t
      {
          /** 数据标识，对应OneNET数据流或资源定义。*/
          const uint8_t *      data_id;
          /** 数据时间戳，UNIX标准时间戳格式。上传时为空则平台自动以收到数据的时间为时间戳*/
          uint32_t             timestamp;
          /** 数据类型*/
          enum iot_data_type_e data_type;
          /** 数据缓冲区*/
          union iot_data_buf_u buf;
      };
      ```
  
  - 接口说明
  
    - 打开设备
  
      接口主要用于初始化相关资源，并使用传递的参数登录OneNET平台。
  
      ```c
      typedef int32_t (*iot_dev_event_cb)(enum iot_dev_event_e /*event*/, void * /*data*/, uint32_t /*data_len*/);
      
      /**
       * @brief 打开一个OneNET设备。
       *
       * @param proto 指定登录平台需要使用的接入协议
       * @param cloud 配置设备使用的OneNET接入服务器及设备信息
       * @param event_handle 设置设备事件处理回调接口
       * @param timeout_ms 超时时间
       * @return void* 设备操作句柄
       */
      void *iot_dev_open(enum iot_proto_e proto, struct iot_cloud_config_t *cloud, iot_dev_event_cb event_handle, uint32_t timeout_ms)
      ```
  
    - 获取设备信息（未使用）
  
    - 关闭设备
  
      登出平台，并清理相关资源。
  
      ```c
      /**
       * @brief 关闭指定的OneNET设备
       *
       * @param dev 设备操作句柄
       * @param timeout_ms 超时时间
       * @retval 0 - 操作成功
       */
      int32_t iot_dev_close(void *dev, uint32_t timeout_ms)
      ```
    
    - 主循环
    
      接口内部在设定的超时时间内，持续尝试从网络获取平台下发的数据并进行解析。当成功解析到下发命令时，通过`iot_dev_open`设定的回调函数通知用户进行处理。
    
      ```c
      /**
       * @brief 主循环
       *
       * @param dev 设备操作句柄
       * @param timeout_ms 处理超时时间。为0时表示阻塞执行，PLAT_MULTI_TASK开启时有效
       * @return int32_t
       */
      int32_t iot_dev_step(void *dev, uint32_t timeout_ms)
      ```
    
    - 推送数据
    
      接口主要用于将用户数据推送到指定资源（其它设备），适用于**EDP**、**MQTT**接入方式。操作不会触发平台保存推送的数据。
    
      ```c
      /**
       * @brief 推送原始数据
       *
       * @param resource
       * 指定数据的推送目标，MQTT协议时使用topic，EDP协议使用目标设备ID。为空时直接发往云端存储，功能与iot_dev_upload一致。
       * @param raw_data 需要发送的数据缓冲区
       * @param data_len 需要发送的数据长度
       * @return int32_t
       */
      int32_t iot_dev_push_rawdata(void *dev, const uint8_t *resource, void *raw_data, uint32_t data_len, uint32_t timeout_ms);
      ```
    
    - 上报数据到平台
    
      ```c
      /**
       * @brief 上传数据点到平台并存储
       *
       * @param dev 设备操作句柄
       * @param data 需要上传的数据点
       * @return int32_t
       */
      int32_t iot_dev_notify(void *dev, struct iot_data_t *data, uint32_t timeout_ms);
      ```
    
    - 订阅资源
    
      主要应用于**MQTT**、**NB-IoT物联网接入套件**，只有当设备订阅了指定资源后，才能接收到通过该资源下发的数据。
    
      ```c
      /**
       * @brief 订阅指定资源的数据
       *
       * @param dev 设备操作句柄
       * @param resource 需要订阅的资源标识
       * @param timeout_ms 超时时间
       * @return int32_t
       */
      int32_t iot_dev_observe(void *dev, const uint8_t *resource, uint32_t timeout_ms);
      ```
    
    - 取消订阅资源
    
      该接口配合**订阅资源**接口使用。
    
      ```c
      /**
       * @brief 取消已订阅的资源
       *
       * @param dev 设备操作句柄
       * @param resource 需要取消订阅的资源标识
       * @return int32_t
       */
      int32_t iot_dev_observe_cancel(void *dev, const uint8_t *resource);
      ```

## 用户用例

### EDP接入

请参考[edp-example](../../examples/onenet_edp)。

### MQTT接入

请参考[mqtt-example](../../examples/onenet_mqtt)。

### MQTT物联网套件接入

请参考[mqtts-example](../../examples/onenet_mqtts)。

### NB-IoT物联网套件接入

请参考[nbiot-example](../../examples/onenet_nbiot)。

