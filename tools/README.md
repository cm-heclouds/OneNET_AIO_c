# OneNET AIO工具层接口说明



## 概述

工具层为上层应用提供服务，主要包含部分第三方开源库，以及用于适配AT客户端的指令模块接口。用户移植SDK时可基于已有的工具库进行开发，或保证上层应用正常调用的情况下替换为自己的库。

## 工具说明

### 一. TLS加密库

工具层内置了开源的wolfssl加密库，用于**security/tls/tls.c**文件以及**onenet/utils/dev_token.c**文件调用，前者封装统一接口为上层应用提供TLS加密，后者用于计算OneNET平台登录的token。

#### 1.1 移植说明

- 若用户基于内置的加密库开发，在移植不同平台时，可参考[官方文档](https://www.wolfssl.com/docs/)根据不同平台特性修改**tools/wolfssl/port/user_settings.h**文件，并且在配置时选择**Security->TLS Library->wolfSSL**即可。

- 若用户使用自己的加密库，需屏蔽掉**tools和onenet/utils**目录下**build.mk、Config.menu**中原有加密库的配置，并在**security/tls/tls.c**中实现加密数据的收发等功能，以及在**onenet/utils/dev_token.c**中实现token的计算。

### 二. AT客户端

AT接口位于**tools/modadptr/at_client.***文件，支持设备作为AT客户端发送与解析AT命令，主要适用于单片机+通信模组的场景，该场景下用户移植SDK到单片机中，通过AT指令与模组交互实现数据收发。

#### 2.1 接口说明

##### 2.1.1 AT模块初始化

使用AT功能前需调用该接口初始化AT数据缓冲区，并配置串口相关信息。若底层接口无需配置串口信息，则port_config参数可传入NULL；缓冲区大小必须保证能够超过任意一帧AT指令长度。

```c
/**
 * @brief 初始化AT模块
 *
 * @param port_config：串口配置信息
 * @param buf_size：AT命令缓冲区大小
 * @return 成功 - AT结构体指针，失败 - NULL
 */
void *at_init(struct uart_config_t *port_config, uint32_t buf_size);
```

##### 2.1.2 AT模块资源释放

该接口用于释放AT初始化申请的内存。

```c
/**
 * @brief 释放AT初始化资源
 *
 * @param ctx：AT结构体指针
 * @return 无
 */
void at_deinit(void *ctx);
```

##### 2.1.3 设置URC对象

该接口需AT模块初始化完成之后调用，用于设置需要匹配的URC对象（模组主动下发给单片机的数据）。参数urc用于指定需要匹配的URC数据前缀和后缀，其中前缀可以为NULL，但后缀必须存在；参数urc_cnt为需要匹配的URC数据个数；参数callback则为用户传入的回调函数，用于接收URC数据（包含前缀至后缀）。

```c
/**
 * @brief 设置URC对象
 *
 * @param ctx：AT结构体指针
 * @param urc：URC结构体数组指针
 * @param urc_cnt：URC数据个数
 * @param callback：URC数据回调
 * @return 成功 - ERR_OK，失败 - 相关错误码
 */
int32_t at_set_urc_obj(void *ctx, struct at_urc_t *urc, uint32_t urc_cnt, at_urc_cb callback);
```

```c
/* 接口参数定义示例 */
struct at_urc_t urc_table[] = 
{
    {"ready", "\r\n"},
    {"GOT IP", "\r\n"},
    {"+IPD", ":"},
};
uint16_t urc_cnt = sizeof(urc_table) / sizeof(urc_table[0]);

/* URC回调函数示例 */
void at_urc_callback(uint8_t *data, uint16_t data_len)
{
    if (NULL != osl_strstr(data, "ready"))
    {
        //to do
    }
    else if (NULL != osl_strstr(data, "GOT IP"))
    {
        //to do
    }
    else if (NULL != osl_strstr(data, "+IPD"))
    {
        //to do
    }
}
```

##### 2.1.4 AT数据发送

该接口用于调用底层串口发送AT命令，一共分为两种：**at_send_cmd**用于发送AT字符串数据，**at_send_raw_data**用于发送指定长度AT数据，用户可以根据需求进行使用，调用该接口前需实现系统适配层uart串口相关配置。

```c
/**
 * @brief 发送AT指令
 *
 * @param ctx：AT结构体指针
 * @param cmd：需要发送的AT字符串
 * @param timeout_ms：超时时间
 * @return >0 - 成功发送的数据长度，其他 - 发送失败
 */
int32_t at_send_cmd(void *ctx, const uint8_t *cmd, uint32_t timeout_ms);

/**
 * @brief 发送指定字节长度AT数据
 *
 * @param ctx：AT结构体指针
 * @param data：需要发送的数据
 * @param data_len：需要发送的数据长度
 * @param timeout_ms：超时时间
 * @return >0 - 成功发送的数据长度，其他 - 发送失败
 */
int32_t at_send_raw_data(void *ctx, uint8_t *data, uint32_t data_len, uint32_t timeout_ms);
```

##### 2.1.5 解析AT响应数据

该接口用于获取并解析是否存在指定后缀的AT响应数据，若存在，则通过指针参数返回指定后缀结尾的一帧或多帧AT响应数据。接口实现的原理是在超时时间内不断将串口接收缓冲区数据按单字节读取到AT缓冲区解析，若解析到指定的响应后缀，则赋值参数指针指向AT响应数据头，并返回成功，期间如果有URC数据，则通过URC回调通知用户。该接口需配合AT数据发送使用，即发送完数据后调用接口获取指定的响应数据。若只需匹配响应后缀，而不关心响应的具体数据，则data和data_len参数传入NULL即可。

```c
/**
 * @brief 获取并解析指定的AT响应数据（发送AT指令后调用）
 *
 * @param ctx：AT结构体指针
 * @param suffix：指定响应后缀，传入NULL则默认后缀为"\r\n"
 * @param data：数据指针，用于指向AT响应数据
 * @param data_len：响应数据长度
 * @param timeout_ms：超时时间
 * @return 成功 - ERR_OK，失败 - 相关错误码
 */
int32_t at_parse_resp(void *ctx, const uint8_t *suffix, uint8_t **data, uint16_t *data_len, uint32_t timeout_ms);
```

```c
/* 接口使用如下示例 */
uint8_t *data = NULL;
uint16_t data_len = 0;
at_send_cmd(ctx, "AT+CWQAP\r\n", 3000); //AT数据发送
if (ERR_OK == at_parse_resp(ctx, "OK\r\n", &data, &data_len, 3000)) //AT响应解析
{
    /* 根据需要处理data参数指向的AT响应数据 */
}
```

**注意：**需要在下一次发送与解析AT、URC数据之前及时处理当前响应的AT数据，否则会因为AT缓冲区被清空而导致数据丢失。

##### 2.1.6 解析URC数据

该接口用于解析是否存在指定前后缀的URC数据，若存在，则通过URC回调函数返回该数据，用户可在回调函数中进行判断和处理。接口实现的原理是在超时时间内不断将串口接收缓冲区数据按单字节读取到AT缓冲区解析，若解析到URC对象，则调用回调函数通知用户，并返回成功。该接口需要在主动处理URC数据的时候循环调用，对于多线程的情况，必须进行加锁，具体后文予以说明。

```c
/**
 * @brief 获取并解析URC数据（循环调用），匹配到指定URC数据后通过回调返回
 *
 * @param ctx：AT结构体指针
 * @param timeout_ms：超时时间
 * @return 成功 - ERR_OK，失败 - 相关错误码
 */
int32_t at_urc_step(void *ctx, uint32_t timeout_ms);
```

**注意：**当解析到URC数据时，用户需及时在回调函数中处理，否则接口退出之后将导致数据丢失。

##### 2.1.7 AT数据接收

该接口用于读取串口接收缓冲区中的AT数据，使用前需实现系统适配层uart串口相关配置。接口主要配合URC回调函数使用，用于匹配到URC对象时取出缓冲区剩余的URC数据。

```c
/**
 * @brief 获取指定字节长度AT数据
 *
 * @param ctx：AT结构体指针
 * @param data：获取数据的存放地址
 * @param data_len：需要获取的数据长度
 * @param timeout_ms：超时时间
 * @return >0 - 成功获取的数据长度，其他 - 获取失败
 */
int32_t at_recv_raw_data(void *ctx, uint8_t *buf, uint32_t buf_len, uint32_t timeout_ms);
```

#### 2.2 多线程加锁说明

对于上文**2.1.4**、**2.1.5**、**2.1.6**接口在多线程中调用时，需进行加锁，否则会影响AT数据的解析和处理，具体的加锁方法满足以下两点：

- **调用2.1.6接口前加锁，调用完毕解锁；**

- **调用2.1.4接口前加锁，然后调用2.1.5接口解析完响应之后解锁。**

如下伪代码示例展现了加锁与解锁过程：

```c
/* 线程一 */
void at_cmd_task(void *arg)
{
    xSemaphoreTake(xSemaphore, portMAX_DELAY); //加锁
    at_send_cmd(at_ctx, "AT+CWQAP\r\n", 3000); //AT数据发送
    at_parse_resp(at_ctx, "OK\r\n", NULL, NULL, 3000); //AT响应解析
    xSemaphoreGive(xSemaphore); //解锁
    vTaskDelay(20 / portTICK_RATE_MS);
}

/* 线程二 */
void at_urc_task(void *arg)
{
    xSemaphoreTake(xSemaphore, portMAX_DELAY); //加锁
    at_urc_step(at_ctx, 50); //URC数据解析
    xSemaphoreGive(xSemaphore); //解锁
    vTaskDelay(20 / portTICK_RATE_MS);
}
```

