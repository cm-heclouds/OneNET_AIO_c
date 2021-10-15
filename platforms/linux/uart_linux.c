/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file uart_linux.c
 * @date 2020/07/31
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <pthread.h>

#include "log.h"
#include "plat_uart.h"
#include "err_def.h"
#include "plat_time.h"
/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define UART_DEVICE_NAME "/dev/ttyUSB0"

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
/** 波特率对应的Flag值*/
static int32_t s_baudrate_flag[] = {B1200,   B1800,   B2400,   B4800,   B9600,   B19200,  B38400, B57600,
                                    B115200, B230400, B460800, B500000, B576000, B921600, -1};

/** 波特率实际值，范围*/
static int32_t s_baudrate[] = {1200,   1800,   2400,   4800,   9600,   19200,  38400, 57600,
                               115200, 230400, 460800, 500000, 576000, 921600, -1};

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
/**
 * @brief 设置波特率
 *
 * @param tios 串口属性结构体指针
 * @param br 波特率
 *
 * @return
 */
static int32_t set_baudrate(struct termios *tios, int32_t br)
{
    int i = 0;

    /* 检查波特率是否有效*/
    while (s_baudrate[i] != -1)
    {
        if (s_baudrate[i] == br)
            break;
        i++;
    }
    if (s_baudrate[i] == -1)
        return ERR_INVALID_PARAM;

    /* 使用波特率对应的Flag值*/
    cfsetispeed(tios, s_baudrate_flag[i]);
    cfsetospeed(tios, s_baudrate_flag[i]);

    return ERR_OK;
}

/**
 * @brief 设置数据位宽
 *
 * @param tios 串口属性结构体指针
 * @param ds 数据位宽
 *
 * @return
 */
static int32_t set_data_bits(struct termios *tios, int32_t ds)
{
    int dsize = 0;

    switch (ds)
    {
        case UART_DATABITS_5:
            dsize = CS5;
            break;
        case UART_DATABITS_6:
            dsize = CS6;
            break;
        case UART_DATABITS_7:
            dsize = CS7;
            break;
        case UART_DATABITS_8:
            dsize = CS8;
            break;
        default:
            return ERR_INVALID_PARAM;
    }

    tios->c_cflag &= ~CSIZE;
    tios->c_cflag |= dsize;

    return ERR_OK;
}

/**
 * @brief 设置奇偶校验
 *
 * @param tios 串口属性结构体指针
 * @param p 校验模式，参见UART_PARITY_XXX
 */
static void set_parity(struct termios *tios, int32_t p)
{
    switch (p)
    {
        case UART_PARITY_NONE: /* 不校验*/
            tios->c_cflag &= ~PARENB;
            tios->c_iflag &= ~INPCK;
            break;
        case UART_PARITY_ODD: /* 奇校验*/
            tios->c_cflag |= PARENB;
            tios->c_cflag |= PARODD;
            tios->c_iflag |= (INPCK | ISTRIP);
            break;
        case UART_PARITY_EVEN: /* 偶校验*/
            tios->c_cflag |= PARENB;
            tios->c_cflag &= ~PARODD;
            tios->c_iflag |= (INPCK | ISTRIP);
            break;
        default:
            break;
    }
}

/**
 * @brief 设置停止位
 *
 * @param tios 串口属性结构体指针
 * @param s 参见UART_STOPBITS_XXX
 */
static void set_stopbits(struct termios *tios, int32_t s)
{
    if (s)
        tios->c_cflag |= CSTOPB;
    else
        tios->c_cflag &= ~CSTOPB;
}

handle_t uart_open(struct uart_config_t *port_config)
{
    int32_t        fd    = -1;
    int32_t        flags = 0;
    struct termios tios;
    int32_t        ret = ERR_OK;

    if (UART_STOPBITS_1_5 == port_config->stop_bits)
    {
        goto exit;
    }

    fd = open(UART_DEVICE_NAME, O_RDWR | O_NOCTTY | O_NDELAY);
    if (0 > fd)
    {
        goto exit;
    }

    tcgetattr(fd, &tios);

    tios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tios.c_iflag &= ~(IXON | IXOFF | IXANY);
    /*防止输入的回车被翻译为换行*/
    tios.c_iflag &= ~ICRNL;

    tios.c_oflag &= ~OPOST;

    tios.c_cflag |= (CLOCAL | CREAD);

    /* 设置波特率*/
    if (ERR_INVALID_PARAM == set_baudrate(&tios, port_config->baudrate))
    {
        goto exit1;
    }
    /* 设置数据位宽*/
    if (ERR_INVALID_PARAM == set_data_bits(&tios, port_config->data_bits))
    {
        goto exit1;
    }

    set_parity(&tios, port_config->parity);
    set_stopbits(&tios, port_config->stop_bits);

    /* 设置立即生效*/
    if (0 > tcsetattr(fd, TCSANOW, &tios))
    {
        goto exit1;
    }

    // fcntl(fd, F_SETFL, 0);

    /* 设置为非阻塞模式*/
    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
        goto exit1;
    }
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    return (handle_t)fd;

exit1:
    close(fd);
exit:
    return -1;
}

int32_t uart_send(handle_t uart, uint8_t *data, uint32_t data_len, uint32_t timeout_ms)
{
    int32_t        fd        = (int32_t)uart;
    int32_t        data_left = data_len;
    int32_t        ret       = -1;
    struct timeval tv;
    fd_set         fs;
    handle_t       cd_hdl = countdown_start(timeout_ms);

    if (data_len == 0)
    {
        return 0;
    }

    do
    {
        FD_ZERO(&fs);
        FD_SET(fd, &fs);
        tv.tv_usec = (countdown_left(cd_hdl) % 1000) * 1000;
        tv.tv_sec  = (countdown_left(cd_hdl) / 1000);

        ret = select(fd + 1, NULL, &fs, NULL, &tv);
        if (-1 == ret)
        {
            countdown_stop(cd_hdl);
            return ERR_IO;
        }
        else if (ret)
        {
            if (FD_ISSET(fd, &fs))
            {
                ret = write(fd, (data + data_len - data_left), data_left);
                if (ret > 0)
                {
                    data_left -= ret;
                }
                else if (EAGAIN != errno)
                {
                    countdown_stop(cd_hdl);
                    return ERR_IO;
                }
            }
        }
    } while ((0 != data_left) && (0 == countdown_is_expired(cd_hdl)));
    countdown_stop(cd_hdl);
    return (data_len - data_left);
}

int32_t uart_recv(handle_t uart, uint8_t *buf, uint32_t buf_len, uint32_t timeout_ms)
{
    int32_t        fd       = (int32_t)uart;
    int32_t        ret      = -1;
    int32_t        recv_len = 0;
    struct timeval tv;
    fd_set         fs;
    handle_t       cd_hdl = countdown_start(timeout_ms);

    do
    {
        FD_ZERO(&fs);
        FD_SET(fd, &fs);
        tv.tv_usec = (countdown_left(cd_hdl) % 1000) * 1000;
        tv.tv_sec  = (countdown_left(cd_hdl) / 1000);
        ret        = select(fd + 1, &fs, NULL, NULL, &tv);
        if (-1 == ret)
        {
            countdown_stop(cd_hdl);
            return ERR_IO;
        }
        else if (0 < ret)
        {
            if (FD_ISSET(fd, &fs))
            {
                ret = read(fd, buf + recv_len, buf_len - recv_len);
                if (ret > 0)
                {
                    recv_len += ret;
                }
                else if (EAGAIN != errno)
                {
                    countdown_stop(cd_hdl);
                    return ERR_IO;
                }
            }
        }
    } while ((recv_len != buf_len) && (0 == countdown_is_expired(cd_hdl)));

    countdown_stop(cd_hdl);
    return recv_len;
}

void uart_close(handle_t uart)
{
    close((int32_t)uart);
}
