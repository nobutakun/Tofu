/**
 * @file translation_cache_layer_additional.c
 * @brief Multi-level cache implementation for Translation Cache Layer
 */

#include "translation_cache_layer.h"
#include "tcl_state.h"
#include "tcl_redis.h"
#include <string.h>
#include <stdlib.h>

// Static function declarations
static tcl_status_t init_memory_cache(tcl_memory_cache_t *cache);
static tcl_status_t init_redis_cache(tcl_redis_cache_t *cache);
static tcl_status_t init_persistent_cache(tcl_persistent_cache_t *cache);
static void update_cache_metrics(tcl_metrics_t *metrics, bool hit, uint64_t response_time);

tcl_status_t tcl_init_multi_level_cache(tcl_multi_level_cache_t *cache) {
    TCL_RETURN_IF_NULL(cache, "Cache pointer is NULL");
    
    // Initialize memory cache
    cache->memory_cache = (tcl_memory_cache_t *)malloc(sizeof(tcl_memory_cache_t));
    if (!cache->memory_cache) {
        return TCL_STATUS_ERROR_MEMORY;
    }
    TCL_RETURN_IF_ERROR(init_memory_cache(cache->memory_cache));
    
    // Initialize Redis cache
    cache->redis_cache = (tcl_redis_cache_t *)malloc(sizeof(tcl_redis_cache_t));
    if (!cache->redis_cache) {
        free(cache->memory_cache);
        return TCL_STATUS_ERROR_MEMORY;
    }
    TCL_RETURN_IF_ERROR(init_redis_cache(cache->redis_cache));
    
    // Initialize persistent cache
    cache->persistent_cache = (tcl_persistent_cache_t *)malloc(sizeof(tcl_persistent_cache_t));
    if (!cache->persistent_cache) {
        free(cache->memory_cache);
        free(cache->redis_cache);
        return TCL_STATUS_ERROR_MEMORY;
    }
    TCL_RETURN_IF_ERROR(init_persistent_cache(cache->persistent_cache));
    
    // Initialize total metrics
    memset(&cache->total_metrics, 0, sizeof(tcl_metrics_t));
    
    return TCL_STATUS_OK;
}

tcl_status_t tcl_cleanup_multi_level_cache(tcl_multi_level_cache_t *cache) {
    TCL_RETURN_IF_NULL(cache, "Cache pointer is NULL");
    
    if (cache->memory_cache) {
        free(cache->memory_cache->entries);
        free(cache->memory_cache);
    }
    
    if (cache->redis_cache) {
        // Just deinitialize Redis - it will handle its own cleanup
        tcl_redis_deinit();
        free(cache->redis_cache);
    }
    
    if (cache->persistent_cache) {
        if (cache->persistent_cache->db_conn) {
            // TODO: Implement database disconnect
        }
        free(cache->persistent_cache);
    }
    
    return TCL_STATUS_OK;
}

tcl_status_t tcl_get_entry(tcl_multi_level_cache_t *cache, const char *key, tcl_entry_t *entry) {
    TCL_RETURN_IF_NULL(cache, "Cache pointer is NULL");
    TCL_RETURN_IF_NULL(key, "Key is NULL");
    TCL_RETURN_IF_NULL(entry, "Entry pointer is NULL");
    
    uint64_t start_time = tcl_get_time_ms();
    bool found = false;
    
    // Try memory cache first
    tcl_status_t status = tcl_memory_cache_get(cache->memory_cache, key, entry);
    if (status == TCL_STATUS_OK) {
        update_cache_metrics(&cache->memory_cache->metrics, true, tcl_get_time_ms() - start_time);
        return TCL_STATUS_OK;
    }
    
    // Try Redis cache
    status = tcl_redis_cache_get(cache->redis_cache, key, entry);
    if (status == TCL_STATUS_OK) {
        // Promote to memory cache
        tcl_memory_cache_set(cache->memory_cache, entry);
        update_cache_metrics(&cache->redis_cache->metrics, true, tcl_get_time_ms() - start_time);
        return TCL_STATUS_OK;
    }
    
    // Try persistent cache
    status = tcl_persistent_cache_get(cache->persistent_cache, key, entry);
    if (status == TCL_STATUS_OK) {
        // Promote to Redis and memory cache
        tcl_redis_cache_set(cache->redis_cache, entry);
        tcl_memory_cache_set(cache->memory_cache, entry);
        update_cache_metrics(&cache->persistent_cache->metrics, true, tcl_get_time_ms() - start_time);
        return TCL_STATUS_OK;
    }
    
    // Entry not found in any cache
    update_cache_metrics(&cache->total_metrics, false, tcl_get_time_ms() - start_time);
    return TCL_STATUS_ERROR_NOT_FOUND;
}

tcl_status_t tcl_set_entry(tcl_multi_level_cache_t *cache, const tcl_entry_t *entry) {
    TCL_RETURN_IF_NULL(cache, "Cache pointer is NULL");
    TCL_RETURN_IF_NULL(entry, "Entry is NULL");
    
    // Set in memory cache
    tcl_status_t status = tcl_memory_cache_set(cache->memory_cache, entry);
    if (status != TCL_STATUS_OK) {
        return status;
    }
    
    // Set in Redis cache
    status = tcl_redis_cache_set(cache->redis_cache, entry);
    if (status != TCL_STATUS_OK) {
        TCL_LOG("Failed to set entry in Redis cache: %d", status);
    }
    
    // Set in persistent cache
    status = tcl_persistent_cache_set(cache->persistent_cache, entry);
    if (status != TCL_STATUS_OK) {
        TCL_LOG("Failed to set entry in persistent cache: %d", status);
    }
    
    return TCL_STATUS_OK;
}

tcl_status_t tcl_update_entry(tcl_multi_level_cache_t *cache, const tcl_entry_t *entry) {
    TCL_RETURN_IF_NULL(cache, "Cache pointer is NULL");
    TCL_RETURN_IF_NULL(entry, "Entry is NULL");
    
    // Update in memory cache
    tcl_status_t status = tcl_memory_cache_update(cache->memory_cache, entry);
    if (status != TCL_STATUS_OK && status != TCL_STATUS_ERROR_NOT_FOUND) {
        return status;
    }
    
    // Update in Redis cache
    status = tcl_redis_cache_update(cache->redis_cache, entry);
    if (status != TCL_STATUS_OK && status != TCL_STATUS_ERROR_NOT_FOUND) {
        return status;
    }
    
    // Update in persistent cache
    status = tcl_persistent_cache_update(cache->persistent_cache, entry);
    if (status != TCL_STATUS_OK) {
        return status;
    }
    
    return TCL_STATUS_OK;
}

tcl_status_t tcl_delete_entry(tcl_multi_level_cache_t *cache, const char *key) {
    TCL_RETURN_IF_NULL(cache, "Cache pointer is NULL");
    TCL_RETURN_IF_NULL(key, "Key is NULL");
    
    // Delete from memory cache
    tcl_status_t status = tcl_memory_cache_delete(cache->memory_cache, key);
    if (status != TCL_STATUS_OK && status != TCL_STATUS_ERROR_NOT_FOUND) {
        return status;
    }
    
    // Delete from Redis cache
    status = tcl_redis_cache_delete(cache->redis_cache, key);
    if (status != TCL_STATUS_OK && status != TCL_STATUS_ERROR_NOT_FOUND) {
        return status;
    }
    
    // Delete from persistent cache
    status = tcl_persistent_cache_delete(cache->persistent_cache, key);
    if (status != TCL_STATUS_OK) {
        return status;
    }
    
    return TCL_STATUS_OK;
}

tcl_status_t tcl_warm_cache(tcl_multi_level_cache_t *cache, const char *usage_data_path, uint32_t preload_count) {
    TCL_RETURN_IF_NULL(cache, "Cache pointer is NULL");
    TCL_RETURN_IF_NULL(usage_data_path, "Usage data path is NULL");
    
    // TODO: Implement cache warming logic
    // 1. Read usage data from the provided path
    // 2. Sort entries by usage count/frequency
    // 3. Preload most frequently used entries into caches
    
    return TCL_STATUS_OK;
}

tcl_status_t tcl_evict_expired_entries(tcl_multi_level_cache_t *cache) {
    TCL_RETURN_IF_NULL(cache, "Cache pointer is NULL");
    
    uint64_t current_time = tcl_get_time_ms();
    
    // Evict from memory cache
    tcl_memory_cache_evict_expired(cache->memory_cache, current_time);
    
    // Evict from Redis cache
    tcl_redis_cache_evict_expired(cache->redis_cache, current_time);
    
    // Evict from persistent cache
    tcl_persistent_cache_evict_expired(cache->persistent_cache, current_time);
    
    return TCL_STATUS_OK;
}

tcl_status_t tcl_get_metrics(tcl_multi_level_cache_t *cache, tcl_metrics_t *metrics) {
    TCL_RETURN_IF_NULL(cache, "Cache pointer is NULL");
    TCL_RETURN_IF_NULL(metrics, "Metrics pointer is NULL");
    
    // Combine metrics from all cache levels
    metrics->hits = cache->memory_cache->metrics.hits + 
                   cache->redis_cache->metrics.hits +
                   cache->persistent_cache->metrics.hits;
                   
    metrics->misses = cache->memory_cache->metrics.misses +
                     cache->redis_cache->metrics.misses +
                     cache->persistent_cache->metrics.misses;
                     
    metrics->evictions = cache->memory_cache->metrics.evictions +
                        cache->redis_cache->metrics.evictions +
                        cache->persistent_cache->metrics.evictions;
                        
    metrics->current_size = cache->memory_cache->metrics.current_size +
                           cache->redis_cache->metrics.current_size +
                           cache->persistent_cache->metrics.current_size;
                           
    metrics->peak_size = cache->memory_cache->metrics.peak_size +
                        cache->redis_cache->metrics.peak_size +
                        cache->persistent_cache->metrics.peak_size;
                        
    // Calculate average response time across all cache levels
    uint64_t total_requests = metrics->hits + metrics->misses;
    if (total_requests > 0) {
        metrics->avg_response_time = (cache->memory_cache->metrics.avg_response_time +
                                    cache->redis_cache->metrics.avg_response_time +
                                    cache->persistent_cache->metrics.avg_response_time) / 3.0;
    } else {
        metrics->avg_response_time = 0;
    }
    
    return TCL_STATUS_OK;
}

// Static function implementations
static tcl_status_t init_memory_cache(tcl_memory_cache_t *cache) {
    cache->max_entries = 1000;  // Default value
    cache->current_entries = 0;
    cache->default_ttl = 3600000;  // 1 hour in milliseconds
    
    cache->entries = (tcl_entry_t *)calloc(cache->max_entries, sizeof(tcl_entry_t));
    if (!cache->entries) {
        return TCL_STATUS_ERROR_MEMORY;
    }
    
    memset(&cache->metrics, 0, sizeof(tcl_metrics_t));
    return TCL_STATUS_OK;
}

static tcl_status_t init_redis_cache(tcl_redis_cache_t *cache) {
    // Set default values
    cache->pool_size = TCL_REDIS_DEFAULT_POOL_SIZE;
    cache->timeout_ms = TCL_REDIS_DEFAULT_TIMEOUT_MS;
    
    // Initialize Redis configuration
    tcl_redis_config_t redis_config = {
        .host = "localhost",
        .port = 6379,
        .password = NULL,
        .timeout_ms = cache->timeout_ms,
        .pool_size = cache->pool_size,
        .enable_tls = false,
        .tls_cert_file = NULL
    };
    
    // Initialize Redis
    tcl_status_t status = tcl_redis_init(&redis_config);
    if (status != TCL_STATUS_OK) {
        return status;
    }
    
    memset(&cache->metrics, 0, sizeof(tcl_metrics_t));
    return TCL_STATUS_OK;
}

static tcl_status_t init_persistent_cache(tcl_persistent_cache_t *cache) {
    cache->max_size = 1000000;  // 1 million entries default
    
    // TODO: Initialize database connection
    cache->db_conn = NULL;
    
    memset(&cache->metrics, 0, sizeof(tcl_metrics_t));
    return TCL_STATUS_OK;
}

static void update_cache_metrics(tcl_metrics_t *metrics, bool hit, uint64_t response_time) {
    if (hit) {
        metrics->hits++;
    } else {
        metrics->misses++;
    }
    
    // Update average response time using moving average
    metrics->avg_response_time = (metrics->avg_response_time * (metrics->hits + metrics->misses - 1) + 
                                response_time) / (metrics->hits + metrics->misses);
                                
    // Update peak size if necessary
    if (metrics->current_size > metrics->peak_size) {
        metrics->peak_size = metrics->current_size;
    }
}
