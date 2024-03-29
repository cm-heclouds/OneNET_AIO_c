/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file log.c
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "log.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/

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
void log_dump_data(const uint8_t *function, uint32_t line, uint8_t *data, uint32_t data_len)
{
    uint32_t i = 0;

    printf("[DUMP]%s %d: ", function, line);
    for (i = 0; i < data_len; i++)
    {
        printf("%02x ", data[i]);
    }
    printf("\n");
}
