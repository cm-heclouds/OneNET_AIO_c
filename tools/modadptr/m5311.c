/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file m5311.c
 * @date 2020/07/31
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

#include "at_cmd.h"
#include "m5311.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define CONFIG_M5311_AT_CMD_TIMEOUT_MS 3000
#define CONFIG_M5311_AT_CMD_LEN_MAX    256
#define CONFIG_M5311_VERSION_LEN_MAX   16

#define EVENT_ID_BOOTSTRAP_OK     2
#define EVENT_ID_BOOTSTRAP_FAILED 3
#define EVENT_ID_LOGIN_OK         6
#define EVENT_ID_LOGIN_FAILED     7
#define EVENT_ID_UPDATE_OK        11
#define EVENT_ID_LOGOUT_OK        15
#define EVENT_ID_NOTIFY_OK        26
/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
enum m5311_state_e
{
    M5311_STATE_START = 0,
    M5311_STATE_COMM_PORT_READY,
    M5311_STATE_MODULE_READY,
    M5311_STATE_NET_REGISTERED,
    M5311_STATE_NET_ATTACHED,
    M5311_STATE_NET_READY,
    M5311_STATE_SLEEP,
    M5311_STATE_LOGIN,
    M5311_STATE_RUNNING
};

struct m5311_t
{
    int8_t m5311_ver[CONFIG_M5311_VERSION_LEN_MAX];
    uint8_t cscon : 1;
    uint8_t cereg : 1;
    uint8_t cgatt : 1;
    uint8_t : 5;
    uint8_t state;
    uint8_t reserved;
    void *msg_handle;
    m2m_event_cb callback;
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
struct m5311_t g_m5311 = {0};
/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
static int32_t at_cmd_callback(const int8_t *cmd, uint32_t cmd_len)
{
    uint32_t msg_id = 0;
    void *msg_handle = g_m5311.msg_handle;

    if(NULL != osl_strstr(cmd, "+MIPLREAD:"))
    {
        struct m2m_attr_t attr_temp = {0};

        osl_sscanf(cmd, "+MIPLREAD:0,%d,%d,%d,%d", &(attr_temp.msg_id), &(attr_temp.data_info.res_info.obj_id),
                   &(attr_temp.data_info.res_info.inst_id), &(attr_temp.data_info.res_info.res_id));
        attr_temp.attr_type = M2M_ATTR_READ;
        g_m5311.callback(msg_handle, M2M_EVENT_RES_READ, (void *)&attr_temp, sizeof(attr_temp));
    }
    else if(NULL != osl_strstr(cmd, "+MIPLWRITE:"))
    {
        struct m2m_attr_t attr_temp = {0};
        uint32_t write_data_len = 0;

        osl_sscanf(cmd, "+MIPLWRITE:0,%d,%d,%d,%d,%d,%d", &(attr_temp.msg_id), &(attr_temp.data_info.res_info.obj_id),
                   &(attr_temp.data_info.res_info.inst_id), &(attr_temp.data_info.res_info.res_id),
                   &(attr_temp.data_info.data_type), &write_data_len);
        
        int i = 0;
        uint16_t comma_last=0;
        uint16_t comma_last_but_one = 0;

        for(i = osl_strlen(cmd); i > 0; i--)
        {
            if(cmd[i] == ',')
            {
                if(comma_last == 0)
                {
                    comma_last = i;
                }
                else if(comma_last_but_one == 0)
                {
                    comma_last_but_one = i;
                    break;
                }
            }
        }

        switch(attr_temp.data_info.data_type)
        {
            case M2M_DT_STRING:
            {
                char *str_ptr = osl_malloc(write_data_len + 1);
                if(str_ptr != NULL)
                {
                    osl_memset(str_ptr, 0, write_data_len + 1);
                    osl_memcpy(str_ptr, cmd + (comma_last_but_one + 1), write_data_len);
                    attr_temp.data_info.data.val_str = str_ptr;
                }
                break;
            }
            case M2M_DT_OPAQUE:
            {
                char *str_ptr = osl_malloc(write_data_len + 1);
                if(str_ptr != NULL)
                {
                    osl_memset(str_ptr, 0, write_data_len + 1);
                    osl_memcpy(str_ptr, cmd + (comma_last_but_one + 1), write_data_len);
                    attr_temp.data_info.data.val_opa.data = str_ptr;
                    attr_temp.data_info.data.val_opa.data_len = write_data_len;
                }
                break;
            }
            case M2M_DT_BOOL:
            case M2M_DT_INTEGER:
            {
                uint32_t data_int = 0;
                osl_sscanf(cmd + comma_last_but_one, ",%d,", &data_int);
                attr_temp.data_info.data.val_int = data_int;
                break;
            }
            case M2M_DT_FLOAT:
            {
                float64_t data_float = 0;
                osl_sscanf(cmd + comma_last_but_one, ",%lf,", &data_float);
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
    }
    else if(NULL != osl_strstr(cmd, "+MIPLEXECUTE:"))
    {
        g_m5311.callback(msg_handle, M2M_EVENT_RES_EXECUTE, NULL, 0);
    }
    else if(NULL != osl_strstr(cmd, "*GOTOSLEEP"))
    {
    }
    else if(NULL != osl_strstr(cmd, "*MATWAKEUP"))
    {
    }
    else if(NULL != osl_strstr(cmd, "+MIPLEVENT:"))
    {
        uint32_t event_id = 0;

        osl_sscanf(cmd, "+MIPLEVENT:0,%d", &event_id);
        switch(event_id)
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
                break;
            case EVENT_ID_LOGOUT_OK:
                g_m5311.callback(msg_handle, M2M_EVENT_LOGOUT_OK, NULL, 0);
                break;
            default:
                break;
        }
    }
    return ERR_OK;
}

static int32_t comm_port_init(void)
{
    struct uart_config_t port_config;
    int32_t ret = ERR_OK;

    port_config.baudrate = 115200;
    port_config.data_bits = UART_DATABITS_8;
    port_config.stop_bits = UART_STOPBITS_1;
    port_config.flow_ctrl = UART_FLOWCTRL_NONE;
    port_config.parity = UART_PARITY_NONE;

    return at_port_open(&port_config, at_cmd_callback);
}

const int8_t *m5311_mipl_get_version(uint32_t timeout_ms)
{
    at_send_cmd_with_reply("AT+MIPLVER?\r\n", "+MIPLVER:", g_m5311.m5311_ver, CONFIG_M5311_VERSION_LEN_MAX, timeout_ms);
    
    return g_m5311.m5311_ver;
}

int32_t m5311_init(void *arg, m2m_event_cb callback, uint32_t timeout_ms)
{
    handle_t cd_hdl = countdown_start(timeout_ms);
    int32_t ret = ERR_OK;

    switch(g_m5311.state)
    {
        case M5311_STATE_START:
            if(ERR_OK == (ret = comm_port_init()))
            {
                g_m5311.state = M5311_STATE_COMM_PORT_READY;
            }
            else
            {
                log_error("comm_port_init error: %d", ret);
                return ERR_INITIALIZING;
            }
        case M5311_STATE_COMM_PORT_READY:
            if((0 != countdown_left(cd_hdl))
               && (ERR_OK == (ret = at_send_cmd_with_reply("AT+CMRB\r\n", "OK", NULL, 0, countdown_left(cd_hdl)))))
            {
                time_delay_ms(5000);
                if((0 != countdown_left(cd_hdl))
                   && (ERR_OK == (ret = at_send_cmd_with_reply("AT\r\n", "OK", NULL, 0, countdown_left(cd_hdl)))))
                {
                    g_m5311.state = M5311_STATE_MODULE_READY;
                }
                else
                {
                    return ERR_INITIALIZING;
                }
            }
            else
            {
                return ERR_INITIALIZING;
            }
        case M5311_STATE_MODULE_READY:
            if((0 == countdown_left(cd_hdl))
               || (ERR_OK != (ret = at_send_cmd_with_reply("AT+CEREG=1\r\n", "OK", NULL, 0, countdown_left(cd_hdl)))))
            {
                return ERR_INITIALIZING;
            }
            if((0 != countdown_left(cd_hdl))
               && (ERR_OK
                   == (ret = at_send_cmd_with_reply("AT+CEREG?\r\n", "+CEREG: 1,1", NULL, 0, countdown_left(cd_hdl)))))
            {
                g_m5311.state = M5311_STATE_NET_REGISTERED;
            }
            else
            {
                return ERR_INITIALIZING;
            }
        case M5311_STATE_NET_REGISTERED:
            if((0 != countdown_left(cd_hdl))
               && (ERR_OK
                   == (ret = at_send_cmd_with_reply("AT+CGATT?\r\n", "+CGATT: 1", NULL, 0, countdown_left(cd_hdl)))))
            {
                g_m5311.state = M5311_STATE_NET_ATTACHED;
            }
            else
            {
                return ERR_INITIALIZING;
            }
        case M5311_STATE_NET_ATTACHED:
            g_m5311.state = M5311_STATE_NET_READY;
        default:
            break;
    }
    
    m5311_mipl_get_version(3000);
    g_m5311.msg_handle = arg;
    g_m5311.callback = callback;
    countdown_stop(cd_hdl);
    return ret;
}

int32_t m5311_deinit(void)
{
    at_port_close();

    return ERR_OK;
}

int32_t m5311_mipl_create(const int8_t *host, uint32_t life_time, const int8_t *authcode, const int8_t *psk, uint8_t flag,
                          uint32_t timeout_ms)
{
    int8_t at_buf[256] = {0};
    osl_sprintf(at_buf, "AT+MIPLCREATEEX=%s,%d", host, flag);
    if(authcode != NULL)
    {
        osl_strcat(at_buf, ",");
        osl_strcat(at_buf, authcode);
    }
    if(psk != NULL)
    {
        osl_strcat(at_buf, ",");
        osl_strcat(at_buf, psk);
    }
    osl_strcat(at_buf, "\r\n");
    return at_send_cmd_with_reply(at_buf, "+MIPLCREATEEX:0", NULL, 0, timeout_ms);
}

int32_t m5311_mipl_delete(uint32_t timeout_ms)
{
    return at_send_cmd_with_reply("AT_DELETE\r\n", "OK", NULL, 0, timeout_ms);
}

int32_t m5311_mipl_open(uint32_t life_time, uint32_t timeout_ms)
{
    int8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};
    int32_t ret = ERR_OK;

    osl_sprintf(cmd, "AT+MIPLOPEN=0,%d\r\n", life_time);
    ret = at_send_cmd_with_reply(cmd, "OK", NULL, 0, timeout_ms);
    return ret;
}

int32_t m5311_mipl_close(uint32_t timeout_ms)
{
    return at_send_cmd_with_reply("AT+MIPLCLOSE=0\r\n", "OK", NULL, 0, timeout_ms);
}

int32_t m5311_mipl_addobj(uint32_t obj_id, uint32_t inst_map, uint32_t timeout_ms)
{
    int8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};
    int32_t ret = ERR_OK;
    uint32_t i = 0;
    uint32_t inst_cnt = 0;

    do
    {
        inst_cnt++;
    } while(inst_map >> inst_cnt);

    osl_sprintf(cmd, "AT+MIPLADDOBJ=0,%d,%d,\"", obj_id, inst_cnt);
    for(i = 0; i < inst_cnt; i++)
    {
        if((inst_map >> (inst_cnt - i - 1)) & 0x1)
            osl_strcat(cmd, "1");
        else
            osl_strcat(cmd, "0");
    }
    osl_strcat(cmd, "\",0,0\r\n");

    return at_send_cmd_with_reply(cmd, "OK", NULL, 0, timeout_ms);
}

int32_t m5311_mipl_delobj(uint32_t obj_id, uint32_t timeout_ms)
{
    int8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};
    int32_t ret = ERR_OK;

    osl_sprintf(cmd, "AT+MIPLDELOBJ=0,%d\r\n", obj_id);

    return at_send_cmd_with_reply(cmd, "OK", NULL, 0, timeout_ms);
}

int32_t m5311_mipl_update(uint32_t life_time, boolean update_objs, uint32_t timeout_ms)
{
    int8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};
    int32_t ret = ERR_OK;

    osl_sprintf(cmd, "AT+MIPLUPDATE=0,%d,%d\r\n", life_time, update_objs);

    return at_send_cmd_with_reply(cmd, "OK", NULL, 0, timeout_ms);
}

int32_t m5311_mipl_step(uint32_t timeout_ms)
{
    return at_step(timeout_ms);
}

static int32_t construct_data(int8_t *cmd, struct m2m_data_t *data)
{
    int8_t value[16] = {0};
    switch(data->data_type)
    {
        case M2M_DT_STRING:
            osl_sprintf(cmd + osl_strlen(cmd), "%d,\"%s\",0,0", osl_strlen(data->data.val_str) + 2, data->data.val_str);
            break;
        case M2M_DT_OPAQUE:
            osl_sprintf(cmd + osl_strlen(cmd), "%d,\"", data->data.val_opa.data_len * 2);
            hex_to_str(cmd + osl_strlen(cmd), data->data.val_opa.data, data->data.val_opa.data_len);
            osl_strcat(cmd, "\",0,0");
            break;
        case M2M_DT_INTEGER:
        case M2M_DT_BOOL:
            osl_sprintf(value, "%d", data->data.val_int);
            osl_sprintf(cmd + osl_strlen(cmd), "%d,\"%s\",0,0", osl_strlen(value), value);
            break;
        case M2M_DT_FLOAT:
            osl_sprintf(value, "%f", data->data.val_float);
            osl_sprintf(cmd + osl_strlen(cmd), "%d,\"%s\",0,0", osl_strlen(value), value);
            break;
        default:
            break;
    }
}

static int32_t construct_notify(int8_t *cmd, const int8_t *prefix, uint32_t msg_id, struct m2m_data_t *data,
                                uint32_t ack_id)
{
    int8_t value[16] = {0};
    uint32_t value_len = 0;

    osl_sprintf(cmd, "%s=0,%d,%d,%d,%d,%d,", prefix, msg_id, data->res_info.obj_id, data->res_info.inst_id,
                data->res_info.res_id, data->data_type);
    construct_data(cmd, data);
    if(ack_id)
    {
        osl_sprintf(cmd + osl_strlen(cmd), ",%d", ack_id);
    }
    osl_strcat(cmd, "\r\n");
    return osl_strlen(cmd);
}

static int32_t construct_reply(int8_t *cmd, const int8_t *prefix, uint32_t msg_id, struct m2m_data_t *data,
                               uint32_t result_code)
{
    int8_t value[16] = {0};
    uint32_t value_len = 0;

    osl_sprintf(cmd, "%s=0,%d,%d,%d,%d,%d,%d,", prefix, msg_id, result_code, data->res_info.obj_id,
                data->res_info.inst_id, data->res_info.res_id, data->data_type);
    construct_data(cmd, data);

    osl_strcat(cmd, "\r\n");
    return osl_strlen(cmd);
}

int32_t m5311_mipl_notify_with_reply(uint32_t msg_id, struct m2m_data_t *data, uint32_t timeout_ms)
{
    int8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};
    uint32_t ack_id = 0;

    construct_notify(cmd, "AT+MIPLNOTIFY", msg_id, data, msg_id);

    //at_send_cmd_with_reply(cmd, "+MIPLEVENT:0,26", cmd, CONFIG_M5311_AT_CMD_LEN_MAX, timeout_ms);
    at_send_cmd_with_reply(cmd, "OK", cmd, CONFIG_M5311_AT_CMD_LEN_MAX, timeout_ms);

    osl_memset(cmd, 0, CONFIG_M5311_AT_CMD_LEN_MAX);
    at_send_cmd_with_reply(cmd, "+MIPLEVENT:0,26,%d", cmd, CONFIG_M5311_AT_CMD_LEN_MAX, timeout_ms);
    /** Check Event and ack_id*/
    osl_sscanf(cmd, "+MIPLEVENT:0,26,%d", &ack_id);
    if(msg_id == ack_id)
        return ERR_OK;
    else
        return ERR_REQUEST_FAILED;
}

int32_t m5311_mipl_notify_without_reply(uint32_t msg_id, struct m2m_data_t *data, uint32_t timeout_ms)
{
    return ERR_OK;
}

int32_t m5311_mipl_discovery_resp(uint32_t obj_id, const int8_t *res_list, uint32_t timeout_ms)
{
    int8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};
    int32_t ret = ERR_OK;
    uint32_t res_list_len = 0;
    res_list_len = osl_strlen(res_list);

    osl_sprintf(cmd, "AT+MIPLDISCOVERRSP=0,%d,1,%d,\"%s\"\r\n", obj_id, res_list_len, res_list);
    return at_send_cmd_with_reply(cmd, "OK", NULL, 0, timeout_ms);
}

int32_t m5311_mipl_read_resp(uint32_t msg_id, uint32_t result_code, struct m2m_data_t *data, uint32_t timeout_ms)
{
    int8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};
    uint32_t ack_id = 0;

    construct_reply(cmd, "AT+MIPLREADRSP", msg_id, data, result_code);
    at_send_cmd_with_reply(cmd, "OK", cmd, CONFIG_M5311_AT_CMD_LEN_MAX, timeout_ms);
}

int32_t m5311_mipl_write_resp(uint32_t msg_id, uint32_t result_code, uint32_t timeout_ms)
{
    int8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};

    osl_sprintf(cmd, "AT+MIPLWRITERSP=0,%d,%d\r\n", msg_id, result_code);
    return at_send_cmd_with_reply(cmd, "OK", NULL, 0, timeout_ms);
}

int32_t m5311_mipl_execute_resp(uint32_t msg_id, uint32_t result_code, uint32_t timeout_ms)
{
    int8_t cmd[CONFIG_M5311_AT_CMD_LEN_MAX] = {0};

    osl_sprintf(cmd, "AT+MIPLEXECUTERSP=0,%d,%d\r\n", msg_id, result_code);
    return at_send_cmd_with_reply(cmd, "OK", NULL, 0, timeout_ms);
}
