/**
 * @file translation_cache_layer.h
 * @brief Core definitions for Translation Cache Layer
 */

#ifndef TRANSLATION_CACHE_LAYER_H
#define TRANSLATION_CACHE_LAYER_H

#include <stdint.h>
#include <stdbool.h>

// Status codes
typedef enum {
    TCL_STATUS_OK = 0,
    TCL_STATUS_ERROR_INVALID_PARAM = -1,
    TCL_STATUS_ERROR_MEMORY = -2,
    TCL_STATUS_ERROR_NOT_FOUND = -3,
    TCL_STATUS_ERROR_ALREADY_EXISTS = -4,
    TCL_STATUS_ERROR_NOT_INITIALIZED = -5,
    TCL_STATUS_ERROR_ALREADY_INITIALIZED = -6,
    TCL_STATUS_ERROR_NETWORK = -7,
    TCL_STATUS_ERROR_TIMEOUT = -8,
    TCL_STATUS_ERROR_INTERNAL = -9,
    TCL_STATUS_ERROR_REDIS = -10,
    TCL_STATUS_ERROR_STORAGE = -11,
    TCL_STATUS_ERROR_INVALID_FORMAT = -12,
    TCL_STATUS_ERROR_IO = -13,
    TCL_STATUS_ERROR_FULL = -14,
    TCL_STATUS_ERROR_EMPTY = -15
} tcl_status_t;

// Cache entry structure
typedef struct {
    char *key;
    char *value;
    uint64_t timestamp;
    uint32_t ttl;
    uint32_t flags;
    float confidence;
    char *source_lang;
    char *target_lang;
    void *metadata;
} tcl_entry_t;

// Cache statistics
typedef struct {
    uint64_t hits;
    uint64_t misses;
    uint64_t evictions;
    double avg_response_time;
    uint32_t current_size;
    uint32_t peak_size;
} tcl_metrics_t;

// Memory cache
typedef struct {
    uint32_t max_entries;
    uint32_t current_entries;
    uint32_t default_ttl;
    tcl_entry_t *entries;
    tcl_metrics_t metrics;
} tcl_memory_cache_t;

// Redis cache
typedef struct {
    void *redis_conn;  // Opaque pointer to Redis connection
    uint32_t pool_size;
    uint32_t timeout_ms;
    tcl_metrics_t metrics;
} tcl_redis_cache_t;

// Persistent storage cache
typedef struct {
    void *db_conn;     // Opaque pointer to database connection
    uint32_t max_size;
    tcl_metrics_t metrics;
} tcl_persistent_cache_t;

// Multi-level cache container
typedef struct {
    tcl_memory_cache_t *memory_cache;
    tcl_redis_cache_t *redis_cache;
    tcl_persistent_cache_t *persistent_cache;
    tcl_metrics_t total_metrics;
} tcl_multi_level_cache_t;

// Multi-level cache operations
tcl_status_t tcl_init_multi_level_cache(tcl_multi_level_cache_t *cache);
tcl_status_t tcl_cleanup_multi_level_cache(tcl_multi_level_cache_t *cache);

// Cache operations
tcl_status_t tcl_get_entry(tcl_multi_level_cache_t *cache, const char *key, tcl_entry_t *entry);
tcl_status_t tcl_set_entry(tcl_multi_level_cache_t *cache, const tcl_entry_t *entry);
tcl_status_t tcl_update_entry(tcl_multi_level_cache_t *cache, const tcl_entry_t *entry);
tcl_status_t tcl_delete_entry(tcl_multi_level_cache_t *cache, const char *key);

// Cache maintenance
tcl_status_t tcl_warm_cache(tcl_multi_level_cache_t *cache, const char *usage_data_path, uint32_t preload_count);
tcl_status_t tcl_evict_expired_entries(tcl_multi_level_cache_t *cache);
tcl_status_t tcl_get_metrics(tcl_multi_level_cache_t *cache, tcl_metrics_t *metrics);

// Logging macro
#define TCL_LOG(fmt, ...) \
    sys_log("TCL", fmt, ##__VA_ARGS__)

// Error handling macros
#define TCL_RETURN_IF_NULL(ptr, msg) \
    if ((ptr) == NULL) { \
        tcl_set_last_error(TCL_STATUS_ERROR_INVALID_PARAM, (msg)); \
        return TCL_STATUS_ERROR_INVALID_PARAM; \
    }

#define TCL_RETURN_IF_ERROR(expr) \
    do { \
        tcl_status_t _status = (expr); \
        if (_status != TCL_STATUS_OK) { \
            return _status; \
        } \
    } while (0)

// Error reporting function
void tcl_set_last_error(tcl_status_t status, const char *message);

#endif // TRANSLATION_CACHE_LAYER_H
