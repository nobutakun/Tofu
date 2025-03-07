/**
 * @file tcl_redis_types.h
 * @brief Redis-specific type definitions and constants
 */

#ifndef TCL_REDIS_TYPES_H
#define TCL_REDIS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "translation_cache_layer.h"

// Redis key limitations
#define TCL_REDIS_KEY_MAX_LENGTH 512
#define TCL_REDIS_VALUE_MAX_LENGTH 512000  // 512KB max value size

// Redis reply types
typedef enum {
    REDIS_REPLY_STRING = 1,
    REDIS_REPLY_ARRAY = 2,
    REDIS_REPLY_INTEGER = 3,
    REDIS_REPLY_NIL = 4,
    REDIS_REPLY_STATUS = 5,
    REDIS_REPLY_ERROR = 6
} tcl_redis_reply_type_t;

// Redis reply structure
typedef struct {
    tcl_redis_reply_type_t type;
    char *str;               // String value
    size_t len;             // String length
    int64_t integer;        // Integer value
    struct tcl_redis_reply_t **elements;  // Array elements
    size_t elements_count;   // Number of elements
} tcl_redis_reply_t;

// Opaque Redis context type
typedef struct tcl_redis_context_t tcl_redis_context_t;

// Redis connection functions
tcl_redis_context_t *redis_connect_with_timeout(const char *host, uint16_t port, uint32_t timeout_ms);
tcl_redis_reply_t *redis_command(tcl_redis_context_t *context, const char *format, ...);
bool redis_enable_tls(tcl_redis_context_t *context, const char *cert_file);
void redis_free(tcl_redis_context_t *context);

// Redis reply management
void tcl_redis_free_reply(tcl_redis_reply_t *reply);

// Internal Redis functions
tcl_status_t redis_send_command(tcl_redis_context_t *context, const char *format, ...);
tcl_status_t redis_read_response(tcl_redis_context_t *context, tcl_redis_reply_t **reply);

// Entry serialization
char *tcl_redis_serialize_entry(const tcl_entry_t *entry);
tcl_status_t tcl_redis_parse_entry(const tcl_redis_reply_t *reply, tcl_entry_t *entry);

#endif // TCL_REDIS_TYPES_H
