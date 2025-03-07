/**
 * @file system_manager.c
 * @brief System management implementation
 */

#include "system_manager.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#define CLOCK_MONOTONIC 0
static int clock_gettime(int dummy, struct timespec *ct) {
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    ct->tv_sec = count.QuadPart / freq.QuadPart;
    ct->tv_nsec = ((count.QuadPart % freq.QuadPart) * 1000000000) / freq.QuadPart;
    return 0;
}
#define nanosleep(req, rem) Sleep((req)->tv_sec * 1000 + (req)->tv_nsec / 1000000)
#else
#include <unistd.h>
#endif

// Static variables
static bool system_initialized = false;
static uint64_t system_start_time = 0;

// Level strings for logging
static const char *log_level_strings[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR"
};

void sys_init(void) {
    if (!system_initialized) {
        system_start_time = sys_get_time_ms();
        system_initialized = true;
    }
}

void sys_deinit(void) {
    if (system_initialized) {
        system_initialized = false;
    }
}

uint64_t sys_get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

void sys_delay_ms(uint32_t ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void sys_log_va(const char *module, sys_log_level_t level, const char *format, va_list args) {
    time_t now;
    struct tm *timeinfo;
    char timestamp[20];
    char message[512];
    
    // Get current time
    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    // Format message
    vsnprintf(message, sizeof(message), format, args);

    // Print log entry
    printf("[%s][%s][%s] %s\n",
           timestamp,
           module,
           log_level_strings[level],
           message);
}

void sys_log(const char *module, const char *format, ...) {
    va_list args;
    va_start(args, format);
    sys_log_va(module, SYS_LOG_INFO, format, args);
    va_end(args);
}

void sys_log_level(const char *module, sys_log_level_t level, const char *format, ...) {
    va_list args;
    va_start(args, format);
    sys_log_va(module, level, format, args);
    va_end(args);
}
