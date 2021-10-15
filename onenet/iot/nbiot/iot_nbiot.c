/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file iot_nbiot.c
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "iot_nbiot.h"
#include "plat_osl.h"
#include "err_def.h"
#include "nbiot_m2m.h"
#include "log.h"
#include "plat_time.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define NBIOT_STATE_CHANGE(x, y)                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (x != y)                                                                                                    \
        {                                                                                                              \
            log_debug("nbiot state change: [%d -> %d]", x, y);                                                         \
            x = y;                                                                                                     \
        }                                                                                                              \
    } while (0)
/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
enum nbiot_state_e
{
    NBIOT_STATE_START = 0,
    NBIOT_STATE_NET_READY,
    NBIOT_STATE_LOGGINGIN,
    NBIOT_STATE_NEED_UPDATE,
    NBIOT_STATE_UPDATING,
    NBIOT_STATE_RUNNING
};

struct iot_nbiot_t
{
    uint32_t                   msg_id;
    uint16_t                   state;
    handle_t                   update_tm;
    void *                     m2m_ptr;
    iot_dev_event_cb           evt_cb;
    struct iot_cloud_config_t *cloud_info;
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
static int32_t m2m_event_callback(void *arg, enum m2m_event_e event, uint8_t *evt_data, uint32_t evt_data_len)
{
    struct iot_nbiot_t *nbiot_dev   = (struct iot_nbiot_t *)arg;
    struct m2m_attr_t * nb_attr_ptr = (struct m2m_attr_t *)evt_data;

    switch (event)
    {
        case M2M_EVENT_LOGIN_OK:
        {
            nbiot_dev->evt_cb(IOT_DEV_CLOUD_CONNECTED, evt_data, evt_data_len);
            NBIOT_STATE_CHANGE(nbiot_dev->state, NBIOT_STATE_RUNNING);
            nbiot_dev->update_tm = countdown_start(nbiot_dev->cloud_info->life_time * 1000);

            break;
        }
        case M2M_EVENT_RES_READ:
        {
            nbiot_dev->evt_cb(IOT_DEV_CMD_RECEIVED, evt_data, evt_data_len);
            m2m_read_reply(nb_attr_ptr->msg_id, nb_attr_ptr->attr_result, &(nb_attr_ptr->data_info), 2000);
            break;
        }
        case M2M_EVENT_RES_WRITE:
        {
            nbiot_dev->evt_cb(IOT_DEV_CMD_RECEIVED, evt_data, evt_data_len);
            m2m_write_reply(nb_attr_ptr->msg_id, nb_attr_ptr->attr_result, 2000);
            break;
        }
        case M2M_EVENT_RES_EXECUTE:
            break;
        case M2M_EVENT_UPDATE_OK:
        {
            countdown_set(nbiot_dev->update_tm, nbiot_dev->cloud_info->life_time * 1000);
            NBIOT_STATE_CHANGE(nbiot_dev->state, NBIOT_STATE_RUNNING);
            break;
        }
        case M2M_EVENT_RES_NOTIFY_OK:
        {
            nbiot_dev->evt_cb(IOT_DEV_RESOURCE_PUSHED, evt_data, evt_data_len);
            break;
        }
        default:
            break;
    }

    return 0;
}

void *iot_nbiot_open(struct iot_cloud_config_t *cloud, iot_dev_event_cb event_cb, uint32_t timeout_ms)
{
    struct iot_nbiot_t *nbiot_dev = NULL;

    nbiot_dev = osl_malloc(sizeof(*nbiot_dev));

    if (NULL == nbiot_dev)
    {
        return NULL;
    }

    osl_memset(nbiot_dev, 0, sizeof(*nbiot_dev));
    nbiot_dev->state   = NBIOT_STATE_START;
    nbiot_dev->m2m_ptr = m2m_init(nbiot_dev, cloud->server_param, m2m_event_callback, timeout_ms);

    if (nbiot_dev->m2m_ptr)
    {
        NBIOT_STATE_CHANGE(nbiot_dev->state, NBIOT_STATE_NET_READY);
        nbiot_dev->cloud_info = cloud;
        nbiot_dev->evt_cb     = event_cb;
    }
    else
    {
        osl_free(nbiot_dev);
        nbiot_dev = NULL;
    }

    return nbiot_dev;
}

int32_t iot_nbiot_step(void *dev, uint32_t timeout_ms)
{
    struct iot_nbiot_t *nbiot_dev = (struct iot_nbiot_t *)dev;
    int32_t             ret       = ERR_OTHERS;
    uint64_t            update_timeout;

    if ((nbiot_dev->cloud_info->life_time) < 600)
    {
        update_timeout = nbiot_dev->cloud_info->life_time * 500;
    }
    else
    {
        update_timeout = 600 * 500;
    }

    switch (nbiot_dev->state)
    {
        case NBIOT_STATE_NET_READY:
        {
            ret = m2m_register(nbiot_dev->m2m_ptr, nbiot_dev->cloud_info->auth_code, nbiot_dev->cloud_info->life_time,
                               timeout_ms);
            if (ret == ERR_OK)
            {
                NBIOT_STATE_CHANGE(nbiot_dev->state, NBIOT_STATE_LOGGINGIN);
            }
            else
            {
                iot_nbiot_close(nbiot_dev, timeout_ms);
            }
            break;
        }
        case NBIOT_STATE_NEED_UPDATE:
        {
            //            log_info("need update at: %lld", time_count_ms());
            ret = m2m_update(nbiot_dev->cloud_info->life_time, timeout_ms);
            if (ret == ERR_OK)
            {
                NBIOT_STATE_CHANGE(nbiot_dev->state, NBIOT_STATE_UPDATING);
            }
            else
            {
                NBIOT_STATE_CHANGE(nbiot_dev->state, NBIOT_STATE_RUNNING);
            }
            break;
        }
        case NBIOT_STATE_RUNNING:
        {
            if (countdown_left(nbiot_dev->update_tm) <= update_timeout)
            {
                NBIOT_STATE_CHANGE(nbiot_dev->state, NBIOT_STATE_NEED_UPDATE);
            }
            break;
        }
        default:
        {
            break;
        }
    }
    m2m_step(timeout_ms);

    return ret;
}

int32_t iot_nbiot_push_rawdata(void *dev, const uint8_t *target, void *raw_data, uint32_t data_len, uint32_t timeout_ms)
{
    struct iot_nbiot_t *nbiot_dev = (struct iot_nbiot_t *)dev;
    struct m2m_data_t   m2m_data;

    osl_sscanf(target, (const uint8_t *)"%d_%d_%d", &m2m_data.res_info.obj_id, &m2m_data.res_info.inst_id, &m2m_data.res_info.res_id);
    m2m_data.data_type             = M2M_DT_OPAQUE;
    m2m_data.data.val_opa.data     = raw_data;
    m2m_data.data.val_opa.data_len = data_len;

    return m2m_notify_with_reply(nbiot_dev->msg_id++, &m2m_data, NULL, 0, timeout_ms);
}

int32_t iot_nbiot_notify(void *dev, struct iot_data_t *data, uint32_t timeout_ms)
{
    struct iot_nbiot_t *nbiot_dev = (struct iot_nbiot_t *)dev;
    struct m2m_data_t   m2m_data;

    osl_sscanf(data->data_id, (const uint8_t *)"%d_%d_%d", &m2m_data.res_info.obj_id, &m2m_data.res_info.inst_id,
               &m2m_data.res_info.res_id);
    switch (data->data_type)
    {
        case IOT_DATA_TYPE_STRING:
            m2m_data.data_type    = M2M_DT_STRING;
            m2m_data.data.val_str = data->buf.val_str;
            break;
        case IOT_DATA_TYPE_BINARY:
            m2m_data.data_type             = M2M_DT_OPAQUE;
            m2m_data.data.val_opa.data     = data->buf.val_bin.buf;
            m2m_data.data.val_opa.data_len = data->buf.val_bin.buf_len;
            break;
        case IOT_DATA_TYPE_INT:
            m2m_data.data_type    = M2M_DT_INTEGER;
            m2m_data.data.val_int = data->buf.val_int;
            break;
        case IOT_DATA_TYPE_FLOAT:
            m2m_data.data_type      = M2M_DT_FLOAT;
            m2m_data.data.val_float = data->buf.val_float;
            break;
        default:
            break;
    }

    return m2m_notify_with_reply(++(nbiot_dev->msg_id), &m2m_data, NULL, 0, timeout_ms);
}

int32_t iot_nbiot_observe(void *dev, const uint8_t *resource, uint32_t timeout_ms)
{
    int32_t               ret       = ERR_OTHERS;
    struct m2m_res_info_t res_info  = {0};
    struct iot_nbiot_t *  nbiot_dev = (struct iot_nbiot_t *)dev;

    osl_sscanf(resource, (const uint8_t *)"%d_%d_%d", &res_info.obj_id, &res_info.inst_id, &res_info.res_id);
    ret = m2m_add_res(nbiot_dev->m2m_ptr, &res_info);

    if (ERR_OK != ret)
    {
        log_error("ret:%d", ret);
    }

    return ERR_OK;
}

int32_t iot_nbiot_observe_cancel(void *dev, const uint8_t *resource, uint32_t timeout_ms)
{
    struct m2m_res_info_t res_info  = {0};
    struct iot_nbiot_t *  nbiot_dev = (struct iot_nbiot_t *)dev;

    osl_sscanf(resource, (const uint8_t *)"%d_%d_%d", &res_info.obj_id, &res_info.inst_id, &res_info.res_id);
    m2m_del_res(nbiot_dev->m2m_ptr, &res_info);

    return ERR_OK;
}

int32_t iot_nbiot_close(void *dev, uint32_t timeout_ms)
{
    struct iot_nbiot_t *nbiot_dev = (struct iot_nbiot_t *)dev;

    m2m_deregister(nbiot_dev->m2m_ptr, timeout_ms);
    m2m_deinit(nbiot_dev->m2m_ptr, timeout_ms);
    countdown_stop(nbiot_dev->update_tm);
    osl_free(nbiot_dev);
    nbiot_dev = NULL;

    return ERR_OK;
}
