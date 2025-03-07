/**
 * @file tcl_redis.h
 * @brief Redis integration for Translation Cache Layer
 */

#ifndef TCL_REDIS_H
#define TCL_REDIS_H

#include "tcl_redis_types.h"
#include "translation_cache_layer.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Redis connection configuration
typedef struct {
    const char *host;          // Redis server hostname
    uint16_t port;            // Redis server port
    const char *password;     // Optional password
    uint32_t timeout_ms;      // Connection timeout
    uint32_t pool_size;       // Connection pool size
    bool enable_tls;          // Whether to use TLS
    const char *tls_cert_file; // Optional TLS certificate file
} tcl_redis_config_t;

// Redis connection pool entry
typedef struct {
    tcl_redis_context_t *context;  // Redis connection context
    bool in_use;                  // Whether connection is currently in use
    uint64_t last_used;           // Timestamp of last use
    uint32_t error_count;         // Consecutive error count
} tcl_redis_conn_t;

// Redis connection state
typedef struct {
    tcl_redis_config_t config;
    tcl_redis_conn_t *pool;
    uint32_t pool_size;
    uint32_t active_connections;
    bool initialized;
    uint64_t total_commands;
    uint64_t failed_commands;
    uint64_t reconnections;
} tcl_redis_state_t;

// Default configuration values
#define TCL_REDIS_DEFAULT_TIMEOUT_MS 1000
#define TCL_REDIS_DEFAULT_POOL_SIZE 5
#define TCL_REDIS_KEY_PREFIX "tcl:"
#define TCL_REDIS_MAX_RETRIES 3
#define TCL_REDIS_RECONNECT_DELAY_MS 1000
#define TCL_REDIS_MAX_ERROR_COUNT 3

// Public interface
tcl_status_t tcl_redis_init(const tcl_redis_config_t *config);
tcl_status_t tcl_redis_deinit(void);

tcl_status_t tcl_redis_get(const char *key, tcl_entry_t *entry);
tcl_status_t tcl_redis_set(const char *key, const tcl_entry_t *entry);
tcl_status_t tcl_redis_delete(const char *key);
tcl_status_t tcl_redis_exists(const char *key, bool *exists);

tcl_status_t tcl_redis_flush_all(void);
tcl_status_t tcl_redis_get_stats(uint32_t *total_keys);
tcl_status_t tcl_redis_health_check(void);

// Connection pool management
tcl_status_t tcl_redis_get_connection(tcl_redis_context_t **context);
void tcl_redis_return_connection(tcl_redis_context_t *context);
tcl_status_t tcl_redis_reset_connection(tcl_redis_context_t *context);

// Internal Redis command functions (made available for schema management)
tcl_status_t redis_send_command(tcl_redis_context_t *context,
                              const char *format, ...);
tcl_status_t redis_read_response(tcl_redis_context_t *context,
                               tcl_redis_reply_t **reply);
void tcl_redis_free_reply(tcl_redis_reply_t *reply);

// Utility functions
const char *tcl_redis_status_string(tcl_status_t status);
tcl_status_t tcl_redis_format_key(const char *key, char *buffer, size_t buffer_size);

#endif // TCL_REDIS_H
