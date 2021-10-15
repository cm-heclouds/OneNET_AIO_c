/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file nbiot_m2m.c
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "nbiot_m2m.h"

#include "err_def.h"
#include "plat_osl.h"
#include "log.h"

#ifdef CONFIG_MOD_ADAPTOR_M5311
#include "m5311.h"
#endif

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define IOT_NBIOT_SERVER_ADDR "coap://nbiotbt.heclouds.com"
#define IOT_NBIOT_SERVER_PORT 5683

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
struct m2m_obj_t
{
    int32_t  obj_id;
    uint32_t inst_map;
    int32_t  res_list[CONFIG_IOT_NBIOT_RESOURCES_MAX_CNT_PER_OBJECT];
};

struct m2m_t
{
    struct network_param_t net;
    m2m_event_cb           callback;
    struct m2m_obj_t       obj_list[CONFIG_IOT_NBIOT_OBJECTS_MAX_CNT];
};

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
static void obj_list_cleanup(struct m2m_obj_t *obj_list)
{
    uint16_t i = 0;

    for (i = 0; i < CONFIG_IOT_NBIOT_OBJECTS_MAX_CNT; i++)
    {
        osl_memset(&obj_list[i], -1, sizeof(*obj_list));
        obj_list[i].inst_map = 0;
    }
}

void *m2m_init(void *arg, struct network_param_t *net, m2m_event_cb callback, uint32_t timeout_ms)
{
    int32_t       ret     = ERR_OK;
    struct m2m_t *m2m_ptr = NULL;

    m2m_ptr = osl_malloc(sizeof(*m2m_ptr));
    if (NULL == m2m_ptr)
    {
        return NULL;
    }
    osl_memset(m2m_ptr, 0, sizeof(struct m2m_t));

#ifdef CONFIG_MOD_ADAPTOR_M5311
    if (ERR_OK != (ret = m5311_init(arg, callback, timeout_ms)))
    {
        log_error("m5311_init error, ret = %d\r\n", ret);
        return NULL;
    }
    else
    {
        log_info("m5311_init ok!");
    }
#endif

    obj_list_cleanup(m2m_ptr->obj_list);

    if (net)
    {
        m2m_ptr->net.remote_addr = osl_strdup(net->remote_addr);
        m2m_ptr->net.remote_port = net->remote_port;
        if (net->cert_len)
        {
            m2m_ptr->net.cert_buf.psk = osl_strdup(net->cert_buf.psk);
            m2m_ptr->net.cert_len     = net->cert_len;
        }
    }
    else
    {
        m2m_ptr->net.remote_addr = osl_strdup(IOT_NBIOT_SERVER_ADDR);
        m2m_ptr->net.remote_port = IOT_NBIOT_SERVER_PORT;
    }

    return m2m_ptr;
}

int32_t m2m_deinit(void *arg, uint32_t timeout_ms)
{
    struct m2m_t *m2m_ptr = (struct m2m_t *)arg;
#ifdef CONFIG_MOD_ADAPTOR_M5311
    m5311_deinit();
#endif

    if (m2m_ptr->net.cert_buf.psk)
    {
        osl_free((void *)m2m_ptr->net.cert_buf.psk);
        m2m_ptr->net.cert_buf.psk = NULL;
    }
    osl_memset(m2m_ptr, 0, sizeof(*m2m_ptr));

    return ERR_OK;
}

int32_t m2m_register(void *arg, uint8_t *auth_code, uint32_t life_time, uint32_t timeout_ms)
{
    int32_t       i, j = 0;
    uint8_t       res_list[CONFIG_IOT_NBIOT_RESOURCES_MAX_CNT_PER_OBJECT * 5] = {0};
    struct m2m_t *m2m_ptr                                                     = (struct m2m_t *)arg;

#ifdef CONFIG_MOD_ADAPTOR_M5311
    uint8_t host[64] = {0};
    osl_sprintf(host, (const uint8_t *)"%s:%d", m2m_ptr->net.remote_addr, m2m_ptr->net.remote_port);
    m5311_mipl_create((const uint8_t *)host, life_time, auth_code, m2m_ptr->net.cert_buf.psk, 1, timeout_ms);

    for (i = 0; i < CONFIG_IOT_NBIOT_OBJECTS_MAX_CNT; i++)
    {
        if (-1 != m2m_ptr->obj_list[i].obj_id)
        {
            m5311_mipl_addobj(m2m_ptr->obj_list[i].obj_id, m2m_ptr->obj_list[i].inst_map, timeout_ms);

            osl_memset(res_list, 0, sizeof(res_list));

            for (j = 0; j < CONFIG_IOT_NBIOT_RESOURCES_MAX_CNT_PER_OBJECT; j++)
            {
                if (-1 != m2m_ptr->obj_list[i].res_list[j])
                {
                    osl_sprintf(res_list + osl_strlen(res_list), "%d;", m2m_ptr->obj_list[i].res_list[j]);
                }
            }
            res_list[osl_strlen(res_list) - 1] = '\0';

            m5311_mipl_discovery_resp(m2m_ptr->obj_list[i].obj_id, res_list, timeout_ms);
        }
    }

    return m5311_mipl_open(life_time, timeout_ms);
#else
    return ERR_OTHERS;
#endif
}

int32_t m2m_update(uint32_t life_time, uint32_t timeout_ms)
{
#ifdef CONFIG_MOD_ADAPTOR_M5311
    return m5311_mipl_update(life_time, 0, timeout_ms);
#endif
}

int32_t m2m_deregister(void *arg, uint32_t timeout_ms)
{
    uint32_t      i       = 0;
    struct m2m_t *m2m_ptr = (struct m2m_t *)arg;

#ifdef CONFIG_MOD_ADAPTOR_M5311
    for (i = 0; i < CONFIG_IOT_NBIOT_OBJECTS_MAX_CNT; i++)
    {
        if (-1 != m2m_ptr->obj_list[i].obj_id)
        {
            m5311_mipl_delobj(m2m_ptr->obj_list[i].obj_id, timeout_ms);
        }
    }
    return m5311_mipl_close(timeout_ms);
#endif
}

int32_t m2m_add_res(void *arg, struct m2m_res_info_t *res_info)
{
    uint16_t i           = 0;
    uint16_t j           = 0;
    int32_t  obj_list_id = 0;
    int32_t  res_list_id = 0;

    //    log_info("start add res: %d_%d_%d", res_info->obj_id, res_info->inst_id, res_info->res_id);

    struct m2m_t *m2m_ptr = (struct m2m_t *)arg;

    obj_list_id = -1;
    for (i = 0; i < CONFIG_IOT_NBIOT_OBJECTS_MAX_CNT; i++)
    {
        if ((-1 == obj_list_id) && (-1 == m2m_ptr->obj_list[i].obj_id))
        {
            obj_list_id = i;
        }

        if (m2m_ptr->obj_list[i].obj_id == res_info->obj_id)
        {
            res_list_id = -1;
            for (j = 0; j < CONFIG_IOT_NBIOT_RESOURCES_MAX_CNT_PER_OBJECT; j++)
            {
                if ((-1 == res_list_id) && (-1 == m2m_ptr->obj_list[i].res_list[j]))
                {
                    res_list_id = j;
                }
                if (m2m_ptr->obj_list[i].res_list[j] == res_info->res_id)
                {
                    break;
                }
            }
            if ((j == CONFIG_IOT_NBIOT_RESOURCES_MAX_CNT_PER_OBJECT) && (-1 == res_list_id))
            {
                return ERR_OVERFLOW;
            }
            else
            {
                res_list_id = (j == CONFIG_IOT_NBIOT_RESOURCES_MAX_CNT_PER_OBJECT) ? res_list_id : j;
            }
            break;
        }
    }
    if ((i == CONFIG_IOT_NBIOT_OBJECTS_MAX_CNT) && (-1 == obj_list_id))
    {
        return ERR_OVERFLOW;
    }
    else
    {
        obj_list_id = (i == CONFIG_IOT_NBIOT_OBJECTS_MAX_CNT) ? obj_list_id : i;
    }

    m2m_ptr->obj_list[obj_list_id].obj_id = res_info->obj_id;
    m2m_ptr->obj_list[obj_list_id].inst_map |= (1 << res_info->inst_id);
    m2m_ptr->obj_list[obj_list_id].res_list[res_list_id] = res_info->res_id;

    //    log_info("add res, m2m_ptr->obj_list[%d].res_list[%d], intmap = %d, ", obj_list_id, res_list_id,
    //             m2m_ptr->obj_list[obj_list_id].inst_map);

    return ERR_OK;
}

int32_t m2m_del_res(void *arg, struct m2m_res_info_t *res_info)
{
    uint16_t      i, j = 0;
    uint16_t      clean_obj = 1;
    struct m2m_t *m2m_ptr   = (struct m2m_t *)arg;

    for (i = 0; i < CONFIG_IOT_NBIOT_OBJECTS_MAX_CNT; i++)
    {
        if (m2m_ptr->obj_list[i].obj_id == res_info->obj_id)
        {
            for (j = 0; j < CONFIG_IOT_NBIOT_RESOURCES_MAX_CNT_PER_OBJECT; j++)
            {
                if (m2m_ptr->obj_list[i].res_list[j] == res_info->res_id)
                {
                    m2m_ptr->obj_list[i].res_list[j] = -1;
                    break;
                }
                else if (-1 != m2m_ptr->obj_list[i].res_list[j])
                {
                    clean_obj = 0;
                }
            }
            if (clean_obj)
            {
                m2m_ptr->obj_list[i].obj_id   = -1;
                m2m_ptr->obj_list[i].inst_map = 0;
            }
        }
    }

    return ERR_OK;
}

int32_t m2m_notify_with_reply(uint32_t msg_id, struct m2m_data_t *data, uint8_t *reply, uint32_t *reply_len,
                              uint32_t timeout_ms)
{
#ifdef CONFIG_MOD_ADAPTOR_M5311
    return m5311_mipl_notify_with_reply(msg_id, data, timeout_ms);
#endif
}

int32_t m2m_notify_without_reply(uint32_t msg_id, struct m2m_data_t *data, uint32_t timeout_ms)
{
#ifdef CONFIG_MOD_ADAPTOR_M5311
    return m5311_mipl_notify_without_reply(msg_id, data, timeout_ms);
#endif
}

int32_t m2m_read_reply(uint32_t msg_id, uint32_t result_code, struct m2m_data_t *data, uint32_t timeout_ms)
{
#ifdef CONFIG_MOD_ADAPTOR_M5311
    switch (data->data_type)
    {
        case IOT_DATA_TYPE_STRING:
            data->data_type = M2M_DT_STRING;
            break;
        case IOT_DATA_TYPE_BINARY:
            data->data_type = M2M_DT_OPAQUE;
            break;
        case IOT_DATA_TYPE_INT:
            data->data_type = M2M_DT_INTEGER;
            break;
        case IOT_DATA_TYPE_FLOAT:
            data->data_type = M2M_DT_FLOAT;
            break;
        default:
            break;
    }
    return m5311_mipl_read_resp(msg_id, result_code, data, timeout_ms);
#endif
}

int32_t m2m_write_reply(uint32_t msg_id, uint32_t result_code, uint32_t timeout_ms)
{
#ifdef CONFIG_MOD_ADAPTOR_M5311
    return m5311_mipl_write_resp(msg_id, result_code, timeout_ms);
#endif
}

int32_t m2m_step(uint32_t timeout_ms)
{
#ifdef CONFIG_MOD_ADAPTOR_M5311
    return m5311_mipl_step(timeout_ms);
#endif
}