/**
 * @file tcl_redis.c
 * @brief Implementation of Redis cache operations
 */

#include "tcl_redis.h"
#include "tcl_redis_types.h"
#include "tcl_redis_schema.h"
#include <string.h>

static tcl_redis_state_t redis_state = {0};

tcl_status_t tcl_redis_init(const tcl_redis_config_t *config) {
    if (redis_state.initialized) {
        return TCL_STATUS_ERROR_ALREADY_INITIALIZED;
    }
    
    TCL_RETURN_IF_NULL(config, "Redis configuration is NULL");
    
    // Store configuration
    memcpy(&redis_state.config, config, sizeof(tcl_redis_config_t));
    
    // Initialize connection pool
    redis_state.pool_size = config->pool_size;
    redis_state.pool = calloc(config->pool_size, sizeof(tcl_redis_conn_t));
    if (!redis_state.pool) {
        return TCL_STATUS_ERROR_MEMORY;
    }
    
    // Establish initial connections
    for (uint32_t i = 0; i < config->pool_size; i++) {
        tcl_redis_context_t *context = redis_connect(config);
        if (!context) {
            tcl_redis_deinit();
            return TCL_STATUS_ERROR_REDIS;
        }
        redis_state.pool[i].context = context;
        redis_state.pool[i].in_use = false;
        redis_state.pool[i].last_used = 0;
        redis_state.pool[i].error_count = 0;
    }
    
    redis_state.initialized = true;
    redis_state.active_connections = config->pool_size;
    
    return TCL_STATUS_OK;
}

tcl_status_t tcl_redis_deinit(void) {
    if (!redis_state.initialized) {
        return TCL_STATUS_ERROR_NOT_INITIALIZED;
    }
    
    // Close all connections in the pool
    for (uint32_t i = 0; i < redis_state.pool_size; i++) {
        if (redis_state.pool[i].context) {
            redis_disconnect(redis_state.pool[i].context);
            redis_state.pool[i].context = NULL;
        }
    }
    
    free(redis_state.pool);
    redis_state.pool = NULL;
    redis_state.initialized = false;
    
    return TCL_STATUS_OK;
}

tcl_status_t tcl_redis_cache_get(const tcl_redis_cache_t *cache, const char *key, tcl_entry_t *entry) {
    TCL_RETURN_IF_NULL(cache, "Cache is NULL");
    TCL_RETURN_IF_NULL(key, "Key is NULL");
    TCL_RETURN_IF_NULL(entry, "Entry is NULL");
    
    tcl_redis_context_t *context;
    TCL_RETURN_IF_ERROR(tcl_redis_get_connection(&context));
    
    char redis_key[TCL_REDIS_KEY_MAX_LENGTH];
    TCL_RETURN_IF_ERROR(tcl_redis_format_key(key, redis_key, sizeof(redis_key)));
    
    tcl_redis_reply_t *reply = NULL;
    tcl_status_t status = redis_send_command(context, "GET %s", redis_key);
    if (status == TCL_STATUS_OK) {
        status = redis_read_response(context, &reply);
        if (status == TCL_STATUS_OK) {
            status = tcl_redis_parse_entry(reply, entry);
        }
    }
    
    tcl_redis_free_reply(reply);
    tcl_redis_return_connection(context);
    
    return status;
}

tcl_status_t tcl_redis_cache_set(const tcl_redis_cache_t *cache, const tcl_entry_t *entry) {
    TCL_RETURN_IF_NULL(cache, "Cache is NULL");
    TCL_RETURN_IF_NULL(entry, "Entry is NULL");
    
    tcl_redis_context_t *context;
    TCL_RETURN_IF_ERROR(tcl_redis_get_connection(&context));
    
    char redis_key[TCL_REDIS_KEY_MAX_LENGTH];
    TCL_RETURN_IF_ERROR(tcl_redis_format_key(entry->key, redis_key, sizeof(redis_key)));
    
    char *entry_str = tcl_redis_serialize_entry(entry);
    if (!entry_str) {
        tcl_redis_return_connection(context);
        return TCL_STATUS_ERROR_MEMORY;
    }
    
    tcl_status_t status = redis_send_command(context, "SETEX %s %u %s",
                                           redis_key,
                                           entry->ttl / 1000, // Convert ms to seconds
                                           entry_str);
    
    free(entry_str);
    tcl_redis_return_connection(context);
    
    return status;
}

tcl_status_t tcl_redis_cache_update(const tcl_redis_cache_t *cache, const tcl_entry_t *entry) {
    // For Redis, update is same as set since SETEX will overwrite
    return tcl_redis_cache_set(cache, entry);
}

tcl_status_t tcl_redis_cache_delete(const tcl_redis_cache_t *cache, const char *key) {
    TCL_RETURN_IF_NULL(cache, "Cache is NULL");
    TCL_RETURN_IF_NULL(key, "Key is NULL");
    
    tcl_redis_context_t *context;
    TCL_RETURN_IF_ERROR(tcl_redis_get_connection(&context));
    
    char redis_key[TCL_REDIS_KEY_MAX_LENGTH];
    TCL_RETURN_IF_ERROR(tcl_redis_format_key(key, redis_key, sizeof(redis_key)));
    
    tcl_status_t status = redis_send_command(context, "DEL %s", redis_key);
    
    tcl_redis_return_connection(context);
    
    return status;
}

tcl_status_t tcl_redis_cache_evict_expired(const tcl_redis_cache_t *cache, uint64_t current_time) {
    // Redis handles TTL expiration automatically
    return TCL_STATUS_OK;
}

// Internal Redis connection management
static tcl_redis_context_t *redis_connect(const tcl_redis_config_t *config) {
    tcl_redis_context_t *context = redis_connect_with_timeout(
        config->host,
        config->port,
        config->timeout_ms
    );
    
    if (!context) {
        return NULL;
    }
    
    if (config->password) {
        tcl_redis_reply_t *reply = redis_command(context, "AUTH %s", config->password);
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            redis_disconnect(context);
            if (reply) tcl_redis_free_reply(reply);
            return NULL;
        }
        tcl_redis_free_reply(reply);
    }
    
    if (config->enable_tls) {
        if (!redis_enable_tls(context, config->tls_cert_file)) {
            redis_disconnect(context);
            return NULL;
        }
    }
    
    return context;
}

static void redis_disconnect(tcl_redis_context_t *context) {
    if (context) {
        redis_free(context);
    }
}
