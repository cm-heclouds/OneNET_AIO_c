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

#include "config.h"
/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define PRODUCT_ID      //"your_product_id"
#define DEVICE_NAME      //"your_dev_name"
#define ACCESS_KEY     //"your_access_key"

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/

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

#if 0 //LBS系统功能点
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

int main(int arg, char **argv[])
{
    struct tm_downlink_tbl_t dl_tbl;

    dl_tbl.prop_tbl = tm_prop_list;
    dl_tbl.prop_tbl_size = tm_prop_list_size;
    dl_tbl.svc_tbl = tm_svc_list;
    dl_tbl.svc_tbl_size = tm_svc_list_size;

    /** 设备初始化*/
    if(0 != tm_init(&dl_tbl))
    {
        goto exit;
    }
    printf("ThingModel init ok\n");

    /** 设备登录*/
    if(0 != tm_login(PRODUCT_ID, DEVICE_NAME, ACCESS_KEY, 3000))
    {
        goto exit;
    }
    printf("ThingModel login ok\n");

    tm_prop_temp_notify(NULL, 27, 0, 3000);

    while (1)
    {
        /** 数据解析*/
        if(0 != tm_step(200))
        {
            break;
        }
        time_delay_ms(50);
    }

    /** 设备注销*/
    tm_logout(3000);

exit:
    return 0;
}
