/**
 * @file system_manager.h
 * @brief System management and logging interface
 */

#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <stddef.h>
#include <stdint.h>

// System initialization/deinitialization
void sys_init(void);
void sys_deinit(void);

// System time management
uint64_t sys_get_time_ms(void);
void sys_delay_ms(uint32_t ms);

// System logging levels
typedef enum {
    SYS_LOG_DEBUG,
    SYS_LOG_INFO,
    SYS_LOG_WARN,
    SYS_LOG_ERROR
} sys_log_level_t;

// System status
typedef enum {
    SYS_STATUS_OK = 0,
    SYS_STATUS_ERROR = -1,
    SYS_STATUS_NOT_INITIALIZED = -2,
    SYS_STATUS_INVALID_PARAM = -3
} sys_status_t;

// Logging functions with levels and module tags
void sys_log(const char *module, const char *format, ...) __attribute__((format(printf, 2, 3)));
void sys_log_level(const char *module, sys_log_level_t level, const char *format, ...) __attribute__((format(printf, 3, 4)));

// Logging macros
#define SYS_LOGD(module, ...) sys_log_level(module, SYS_LOG_DEBUG, __VA_ARGS__)
#define SYS_LOGI(module, ...) sys_log_level(module, SYS_LOG_INFO, __VA_ARGS__)
#define SYS_LOGW(module, ...) sys_log_level(module, SYS_LOG_WARN, __VA_ARGS__)
#define SYS_LOGE(module, ...) sys_log_level(module, SYS_LOG_ERROR, __VA_ARGS__)

#endif // SYSTEM_MANAGER_H
