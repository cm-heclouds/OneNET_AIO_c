/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file m5311.c
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "plat_osl.h"
#include "plat_time.h"
#include "err_def.h"

#include "log.h"
#include "utils.h"

#include "at_client.h"
#include "m5311.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define CONFIG_M5311_AT_CMD_LEN_MAX 256

#define EVENT_ID_BOOTSTRAP_OK     2
#define EVENT_ID_BOOTSTRAP_FAILED 3
#define EVENT_ID_LOGIN_OK         6
#define EVENT_ID_LOGIN_FAILED     7
#define EVENT_ID_UPDATE_OK        11
#define EVENT_ID_LOGOUT_OK        15
#define EVENT_ID_NOTIFY_OK        26

#define AT_RECV_BUF_LEN 512

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
struct m5311_t
{
    void *       msg_handle;
    m2m_event_cb callback;
};

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static void *at_ctx = NULL;

static struct at_urc_t urc_table[] = {
    {"+IP:", "\r\n"},        {"+MIPLEVENT:", "\r\n"},   {"+MIPLREAD:", "\r\n"},
    {"+MIPLWRITE:", "\r\n"}, {"+MIPLEXECUTE:", "\r\n"},
};
static uint16_t urc_cnt = sizeof(urc_table) / sizeof(urc_table[0]);

static uint8_t  module_net_sta;
static uint16_t notify_ack_id;

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
struct m5311_t g_m5311 = {0};

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
static void m5311_at_urc_callback(uint8_t *cmd, uint16_t cmd_len)
{
    void *msg_handle = g_m5311.msg_handle;

    log_debug("at callback: %s\n", cmd);

    if (NULL != osl_strstr(cmd, "+IP:"))
    {
        module_net_sta = 1;
    }
    else if (NULL != osl_strstr(cmd, "+MIPLREAD:"))
    {
        struct m2m_attr_t attr_temp = {0};

        osl_sscanf(cmd, (const uint8_t *)"+MIPLREAD:0,%d,%d,%d,%d", &(attr_temp.msg_id),
                   &(attr_temp.data_info.res_info.obj_id), &(attr_temp.data_info.res_info.inst_id),
                   &(attr_temp.data_info.res_info.res_id));
        attr_temp.attr_type = M2M_ATTR_READ;
        g_m5311.callback(msg_handle, M2M_EVENT_RES_READ, (void *)&attr_temp, sizeof(attr_temp));
    }
    else if (NULL != osl_strstr(cmd, (const uint8_t *)"+MIPLWRITE:"))
    {
        struct m2m_attr_t attr_temp      = {0};
        uint32_t          write_data_len = 0;

        osl_sscanf(cmd, (const uint8_t *)"+MIPLWRITE:0,%d,%d,%d,%d,%d,%d", &(attr_temp.msg_id),
                   &(attr_temp.data_info.res_info.obj_id), &(attr_temp.data_info.res_info.inst_id),
                   &(attr_temp.data_info.res_info.res_id), &(attr_temp.data_info.data_type), &write_data_len);

        int      i                  = 0;
        uint16_t comma_last         = 0;
        uint16_t comma_last_but_one = 0;

        for (i = osl_strlen(cmd); i > 0; i--)
        {
            if (cmd[i] == ',')
            {
                if (comma_last == 0)
                {
                    comma_last = i;
                }
                else if (comma_last_but_one == 0)
                {
                    comma_last_but_one = i;
                    break;
                }
            }
        }

        switch (attr_temp.data_info.data_type)
        {
            case M2M_DT_STRING:
            {
                uint8_t *str_ptr = osl_malloc(write_data_len + 1);
                if (str_ptr != NULL)
                {
                    osl_memset(str_ptr, 0, write_data_len + 1);
                    osl_memcpy(str_ptr, cmd + (comma_last_but_one + 1), write_data_len);
                    attr_temp.data_info.data.val_str = str_ptr;
                }
                break;
            }
            case M2M_DT_OPAQUE:
            {
                uint8_t *str_ptr = osl_malloc(write_data_len + 1);
                if (str_ptr != NULL)
                {
                    osl_memset(str_ptr, 0, write_data_len + 1);
                    osl_memcpy(str_ptr, cmd + (comma_last_but_one + 1), write_data_len);
                    attr_temp.data_info.data.val_opa.data     = str_ptr;
                    attr_temp.data_info.data.val_opa.data_len = write_data_len;
                }
                break;
            }
            case M2M_DT_BOOL:
            case M2M_DT_INTEGER:
            {
                uint32_t data_int = 0;
                osl_sscanf(cmd + comma_last_but_one, (const uint8_t *)",%d,", &data_int);
                attr_temp.data_info.data.val_int = data_int;
                break;
            }
            case M2M_DT_FLOAT:
            {
                float64_t data_float = 0;
                osl_sscanf(cmd + comma_last_but_one, (const uint8_t *)",%lf,", &data_float);
                attr_temp.data_info.data.val_float = data_float;
                break;
            }
            default:
            {
                break;
            }
        }
        attr_temp.attr_type = M2M_ATTR_WRITE;
        g_m5311.callback(msg_handle, M2M_EVENT_RES_WRITE, (void *)&attr_temp, sizeof(attr_temp));
        if ((M2M_DT_OPAQUE == attr_temp.data_info.data_type) && (NULL != attr_temp.data_info.data.val_opa.data))
        {
            osl_free(attr_temp.data_info.data.val_opa.data);
        }
        if ((M2M_DT_STRING == attr_temp.data_info.data_type) && (NULL != attr_temp.data_info.data.val_str))
        {
            osl_free(attr_temp.data_info.data.val_str);
        }
    }
    else if (NULL != osl_strstr(cmd, (const uint8_t *)"+MIPLEXECUTE:"))
    {
        g_m5311.callback(msg_handle, M2M_EVENT_RES_EXECUTE, NULL, 0);
    }
    else if (NULL != osl_strstr(cmd, (const uint8_t *)"+MIPLEVENT:"))
    {
        uint32_t event_id = 0;

        osl_sscanf(cmd, (const uint8_t *)"+MIPLEVENT:0,%d", &event_id);
        switch (event_id)
        {
            case EVENT_ID_BOOTSTRAP_FAILED:
            case EVENT_ID_LOGIN_FAILED:
                g_m5311.callback(msg_handle, M2M_EVENT_LOGIN_FAILED, NULL, 0);
                break;
            case EVENT_ID_BOOTSTRAP_OK:
                log_info("BOOTSTRAP OK\r\n");
                break;
            case EVENT_ID_LOGIN_OK:
                g_m5311.callback(msg_handle, M2M_EVENT_LOGIN_OK, NULL, 0);
                log_info("LOG IN OK\r\n");
                break;
            case EVENT_ID_UPDATE_OK:
                g_m5311.callback(msg_handle, M2M_EVENT_UPDATE_OK, NULL, 0);
                break;
            case EVENT_ID_NOTIFY_OK:
                g_m5311.callback(msg_handle, M2M_EVENT_RES_NOTIFY_OK, NULL, 0);
                osl_sscanf(cmd, (const uint8_t *)"+MIPLEVENT:0,%d,%d", &event_id, &notify_ack_id);
                break;
            case EVENT_ID_LOGOUT_OK:
                g_m5311.callback(msg_handle, M2M_EVENT_LOGOUT_OK, NULL, 0);
                break;
            default:
                break;
        }
    }
}

int32_t m5311_init(void *arg, m2m_event_cb callback, uint32_t timeout_ms)
{
    handle_t cd_hdl = countdown_start(timeout_ms);
    int32_t  ret    = ERR_OK;

    struct uart_config_t port_config;

    port_config.baudrate  = 115200;
    port_config.data_bits = UART_DATABITS_8;
    port_config.stop_bits = UART_STOPBITS_1;
    port_config.flow_ctrl = UART_FLOWCTRL_NONE;
    port_config.parity    = UART_PARITY_NONE;

    if (NULL == (at_ctx = at_init(&port_config, AT_RECV_BUF_LEN))) // AT结构体初始化
    {
        ret = ERR_INITIALIZING;
        goto exit;
    }
    at_set_urc_obj(at_ctx, urc_table, urc_cnt, m5311_at_urc_callback); //设置URC对象

    do
    {
        if (1 == module_net_sta)
        {
            log_info("module net connected\n");
            module_net_sta     = 0;
            g_m5311.msg_handle = arg;
            g_m5311.callback   = callback;
            goto exit;
        }
        at_urc_step(at_ctx, 50);
    } while (0 == countdown_is_expired(cd_hdl));

    at_deinit(at_ctx);
    ret = ERR_TIMEOUT;

exit:
    countdown_stop(cd_hdl);
    return ret;
}

void m5311_deinit(void)
{
    at_deinit(at_ctx);
}

int32_t m5311_mipl_create(const uint8_t *host, uint32_t life_time, const uint8_t *authcode, const uint8_t *psk,
                          uint8_t flag, uint32_t timeout_ms)
{
    uint8_t cmd[256] = {0};

    osl_sprintf(cmd, (const uint8_t *)"AT+MIPLCREATEEX=%s,%d", host, flag);
    if (authcode != NULL)
    {
        osl_strcat(cmd, (const uint8_t *)",");
        osl_strcat(cmd, authcode);
    }
    if (psk != NULL)
    {
        osl_strcat(cmd, (const uint8_t *)",");
        osl_strcat(cmd, psk);
    }
    osl_strcat(cmd, (const uint8_t *)"\r\n");

    at_send_cmd(at_ctx, cmd, timeout_ms);

    return at_parse_resp(at_ctx, "+MIPLCREATEEX:0", NULL, NULL, timeout_ms);
}

int32_t m5311_mipl_delete(uint32_t timeout_ms)
{
    at_send_cmd(at_ctx, "AT+MIPLDELETE=0\r\n", timeout_ms);

    return at_parse_resp(at_ctx, "OK\r\n", NULL, NULL, timeout_ms);
}

int32_t m5311_mipl_open(uint32_t life_time, uint32_t timeout_ms)
{
    uint8_t cmd[32] = {0};

    osl_sprintf(cmd, (const uint8_t *)"AT+MIPLOPEN=0,%d,%d\r\n", life_time, 30);
    at_send_cmd(at_ctx, cmd, timeout_ms);

    return at_parse_resp(at_ctx, "OK\r\n", NULL, NULL, timeout_ms);
}

int32_t m5311_mipl_close(uint32_t timeout_ms)
{
    at_send_cmd(at_ctx, "AT+MIPLCLOSE=0\r\n", timeout_ms);

    return at_parse_resp(at_ctx, "OK\r\n", NULL, NULL, timeout_ms);
}

int32_t m5311_mipl_addobj(uint32_t obj_id, uint32_t inst_map, uint32_t timeout_ms)
{
    uint8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};

    uint32_t i        = 0;
    uint32_t inst_cnt = 0;

    do
    {
        inst_cnt++;
    } while (inst_map >> inst_cnt);

    osl_sprintf(cmd, (const uint8_t *)"AT+MIPLADDOBJ=0,%d,%d,\"", obj_id, inst_cnt);
    for (i = 0; i < inst_cnt; i++)
    {
        if ((inst_map >> (inst_cnt - i - 1)) & 0x1)
            osl_strcat(cmd, (const uint8_t *)"1");
        else
            osl_strcat(cmd, (const uint8_t *)"0");
    }
    osl_strcat(cmd, (const uint8_t *)"\",0,0\r\n");

    at_send_cmd(at_ctx, cmd, timeout_ms);

    return at_parse_resp(at_ctx, "OK\r\n", NULL, NULL, timeout_ms);
}

int32_t m5311_mipl_delobj(uint32_t obj_id, uint32_t timeout_ms)
{
    uint8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};

    osl_sprintf(cmd, (const uint8_t *)"AT+MIPLDELOBJ=0,%d\r\n", obj_id);
    at_send_cmd(at_ctx, cmd, timeout_ms);

    return at_parse_resp(at_ctx, "OK\r\n", NULL, NULL, timeout_ms);
}

int32_t m5311_mipl_update(uint32_t life_time, boolean update_objs, uint32_t timeout_ms)
{
    uint8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};

    osl_sprintf(cmd, (const uint8_t *)"AT+MIPLUPDATE=0,%d,%d\r\n", life_time, update_objs);
    at_send_cmd(at_ctx, cmd, timeout_ms);

    return at_parse_resp(at_ctx, "OK\r\n", NULL, NULL, timeout_ms);
}

int32_t m5311_mipl_step(uint32_t timeout_ms)
{
    return at_urc_step(at_ctx, timeout_ms);
}

static int32_t construct_data(uint8_t *cmd, struct m2m_data_t *data)
{
    uint8_t value[16] = {0};
    switch (data->data_type)
    {
        case M2M_DT_STRING:
            osl_sprintf(cmd + osl_strlen(cmd), (const uint8_t *)"%d,\"%s\",0,0", osl_strlen(data->data.val_str) + 2,
                        data->data.val_str);
            break;
        case M2M_DT_OPAQUE:
            osl_sprintf(cmd + osl_strlen(cmd), (const uint8_t *)"%d,\"", data->data.val_opa.data_len * 2);
            hex_to_str(cmd + osl_strlen(cmd), data->data.val_opa.data, data->data.val_opa.data_len);
            osl_strcat(cmd, (const uint8_t *)"\",0,0");
            break;
        case M2M_DT_INTEGER:
        case M2M_DT_BOOL:
            osl_sprintf(value, (const uint8_t *)"%d", data->data.val_int);
            osl_sprintf(cmd + osl_strlen(cmd), (const uint8_t *)"%d,\"%s\",0,0", osl_strlen(value), value);
            break;
        case M2M_DT_FLOAT:
            osl_sprintf(value, (const uint8_t *)"%f", data->data.val_float);
            osl_sprintf(cmd + osl_strlen(cmd), (const uint8_t *)"%d,\"%s\",0,0", osl_strlen(value), value);
            break;
        default:
            break;
    }

    return 0;
}

static int32_t construct_notify(uint8_t *cmd, const uint8_t *prefix, uint32_t msg_id, struct m2m_data_t *data,
                                uint32_t ack_id)
{
    osl_sprintf(cmd, (const uint8_t *)"%s=0,%d,%d,%d,%d,%d,", prefix, msg_id, data->res_info.obj_id,
                data->res_info.inst_id, data->res_info.res_id, data->data_type);
    construct_data(cmd, data);
    if (ack_id)
    {
        osl_sprintf(cmd + osl_strlen(cmd), (const uint8_t *)",%d", ack_id);
    }
    osl_strcat(cmd, (const uint8_t *)"\r\n");
    return osl_strlen(cmd);
}

static int32_t construct_reply(uint8_t *cmd, const uint8_t *prefix, uint32_t msg_id, struct m2m_data_t *data,
                               uint32_t result_code)
{
    osl_sprintf(cmd, (const uint8_t *)"%s=0,%d,%d,%d,%d,%d,%d,", prefix, msg_id, result_code, data->res_info.obj_id,
                data->res_info.inst_id, data->res_info.res_id, data->data_type);
    construct_data(cmd, data);

    osl_strcat(cmd, (const uint8_t *)"\r\n");
    return osl_strlen(cmd);
}

int32_t m5311_mipl_notify_with_reply(uint32_t msg_id, struct m2m_data_t *data, uint32_t timeout_ms)
{
    handle_t cd_hdl = countdown_start(timeout_ms);
    int32_t  ret    = ERR_TIMEOUT;

    uint8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};
    construct_notify(cmd, (const uint8_t *)"AT+MIPLNOTIFY", msg_id, data, msg_id);

    at_send_cmd(at_ctx, cmd, countdown_left(cd_hdl));
    if (0 == at_parse_resp(at_ctx, "OK\r\n", NULL, NULL, countdown_left(cd_hdl)))
    {
        do
        {
            if (msg_id == notify_ack_id)
            {
                ret = ERR_OK;
                break;
            }
        } while (0 == countdown_is_expired(cd_hdl));
        countdown_stop(cd_hdl);
    }

    return ret;
}

int32_t m5311_mipl_notify_without_reply(uint32_t msg_id, struct m2m_data_t *data, uint32_t timeout_ms)
{
    uint8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};

    construct_notify(cmd, (const uint8_t *)"AT+MIPLNOTIFY", msg_id, data, 0);
    at_send_cmd(at_ctx, cmd, timeout_ms);

    return at_parse_resp(at_ctx, "OK\r\n", NULL, NULL, timeout_ms);
}

int32_t m5311_mipl_discovery_resp(uint32_t obj_id, const uint8_t *res_list, uint32_t timeout_ms)
{
    uint8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};

    uint32_t res_list_len = osl_strlen(res_list);

    osl_sprintf(cmd, (const uint8_t *)"AT+MIPLDISCOVERRSP=0,%d,1,%d,\"%s\"\r\n", obj_id, res_list_len, res_list);
    at_send_cmd(at_ctx, cmd, timeout_ms);

    return at_parse_resp(at_ctx, "OK\r\n", NULL, NULL, timeout_ms);
}

int32_t m5311_mipl_read_resp(uint32_t msg_id, uint32_t result_code, struct m2m_data_t *data, uint32_t timeout_ms)
{
    uint8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};

    construct_reply(cmd, (const uint8_t *)"AT+MIPLREADRSP", msg_id, data, result_code);
    at_send_cmd(at_ctx, cmd, timeout_ms);

    return at_parse_resp(at_ctx, "OK\r\n", NULL, NULL, timeout_ms);
}

int32_t m5311_mipl_write_resp(uint32_t msg_id, uint32_t result_code, uint32_t timeout_ms)
{
    uint8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};

    osl_sprintf(cmd, (const uint8_t *)"AT+MIPLWRITERSP=0,%d,%d\r\n", msg_id, result_code);
    at_send_cmd(at_ctx, cmd, timeout_ms);

    return at_parse_resp(at_ctx, "OK\r\n", NULL, NULL, timeout_ms);
}

int32_t m5311_mipl_execute_resp(uint32_t msg_id, uint32_t result_code, uint32_t timeout_ms)
{
    uint8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};

    osl_sprintf(cmd, (const uint8_t *)"AT+MIPLEXECUTERSP=0,%d,%d\r\n", msg_id, result_code);
    at_send_cmd(at_ctx, cmd, timeout_ms);

    return at_parse_resp(at_ctx, "OK\r\n", NULL, NULL, timeout_ms);
}
