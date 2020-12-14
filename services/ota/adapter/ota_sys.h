#ifndef _OTA_SYS_H_
#define _OTA_SYS_H_

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include "data_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/* External Definition (Constant and Macro)                                 */
/*****************************************************************************/

#define OTA_OK    (1)
#define OTA_ERROR (0)

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/
typedef handle_t ota_handle_t;
/*****************************************************************************/
/* External Variables and Functions                                          */
/*****************************************************************************/

int ota_log_printf(const char *format, ...);
int ota_atoi(const char *nptr);
void ota_hex2str(const char *src_hex, char *Dst_str, int hex_len);
void *ota_malloc(size_t length);
void ota_free(void *p);
void *ota_memset(void *dst, int val, size_t size);
void *ota_memcpy(void *dst, const void *src, size_t n);
char *ota_strcat(char *dst, const char *src);
char *ota_strcpy(char *dst, const char *src);
char *ota_strstr(const char *s1, const char *s2);
uint32_t ota_strlen(const char *s);
int32_t ota_sprintf(char *str, const char *format, ...);

void ota_deinit_free(void *p);

uint8_t ota_scene_recall(void *user_context);
uint8_t ota_scene_store(void *user_context);

/**
 * @brief 计算下次获取升级包的范围
 *
 * @param package_now 目前已经下载的升级包的范围
 * @param start 用于保存下次获取的范围的起始值
 * @param end 用于保存下次获取的范围的终止值
 */
void ota_calculate_segment_range(uint32_t package_now, uint32_t *start, uint32_t *end);

/**
 * @brief 时间计时，单位毫秒
 *
 * @return uint64_t 返回当前毫秒级计数
 */
uint64_t ota_get_tick_ms(void);

#ifdef __cplusplus
}
#endif

#endif //_OTA_SYS_H_
