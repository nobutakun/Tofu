/**
 * @file tcl_entry_manager.c
 * @brief Implementation of cache entry management
 */

#include "tcl_entry_manager.h"
#include "tcl_key_generator.h"
#include "tcl_state.h"
#include "../../system_manager.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>

// Internal state
static struct {
    tcl_entry_manager_config_t config;
    bool initialized;
} entry_manager_state = {
    .initialized = false
};

// Forward declarations of internal functions
static tcl_status_t evict_lru_entries(uint32_t count);
static tcl_status_t evict_lfu_entries(uint32_t count);
static tcl_status_t evict_fifo_entries(uint32_t count);
static tcl_status_t evict_random_entries(uint32_t count);

// Core entry management functions
tcl_status_t tcl_entry_add(tcl_entry_t *entry) {
    if (!entry_manager_state.initialized) {
        tcl_set_last_error(TCL_STATUS_ERROR_NOT_INITIALIZED, 
                          "Entry manager not initialized");
        return TCL_STATUS_ERROR_NOT_INITIALIZED;
    }

    TCL_RETURN_IF_NULL(entry, "Entry is NULL");

    // Check if we need to evict entries
    if (tcl_entry_get_free_space() < 1) {
        TCL_RETURN_IF_ERROR(tcl_entry_evict(
            entry_manager_state.config.eviction_batch_size));
    }

    // Copy entry to cache
    tcl_entry_t *new_entry = &tcl_state.entries[tcl_state.entry_count];
    TCL_RETURN_IF_ERROR(tcl_copy_entry(entry, new_entry));
    
    // Update metadata
    new_entry->timestamp = sys_get_time_ms();
    new_entry->metadata.usage_count = 1;
    new_entry->metadata.last_used = new_entry->timestamp;

    tcl_state.entry_count++;
    TCL_LOG("Added new cache entry, total entries: %u", tcl_state.entry_count);
    return TCL_STATUS_OK;
}

// Eviction policy implementations
static tcl_status_t evict_lru_entries(uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (tcl_state.entry_count == 0) {
            break;
        }

        uint32_t lru_idx = 0;
        uint64_t oldest_access = UINT64_MAX;

        // Find least recently used entry
        for (uint32_t j = 0; j < tcl_state.entry_count; j++) {
            if (tcl_state.entries[j].metadata.last_used < oldest_access) {
                oldest_access = tcl_state.entries[j].metadata.last_used;
                lru_idx = j;
            }
        }

        TCL_LOG("Evicting LRU entry at index %u, last used: %lu", 
                lru_idx, oldest_access);

        tcl_free_entry(&tcl_state.entries[lru_idx]);

        // Move last entry to this position if not already last
        if (lru_idx < tcl_state.entry_count - 1) {
            memmove(&tcl_state.entries[lru_idx],
                   &tcl_state.entries[tcl_state.entry_count - 1],
                   sizeof(tcl_entry_t));
        }

        tcl_state.entry_count--;
        tcl_state.stats.evictions++;
    }

    return TCL_STATUS_OK;
}

static tcl_status_t evict_lfu_entries(uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (tcl_state.entry_count == 0) {
            break;
        }

        uint32_t lfu_idx = 0;
        uint32_t lowest_usage = UINT32_MAX;
        
        for (uint32_t j = 0; j < tcl_state.entry_count; j++) {
            if (tcl_state.entries[j].metadata.usage_count < lowest_usage) {
                lowest_usage = tcl_state.entries[j].metadata.usage_count;
                lfu_idx = j;
            }
        }
        
        tcl_free_entry(&tcl_state.entries[lfu_idx]);
        
        if (lfu_idx < tcl_state.entry_count - 1) {
            memmove(&tcl_state.entries[lfu_idx],
                   &tcl_state.entries[tcl_state.entry_count - 1],
                   sizeof(tcl_entry_t));
        }
        
        tcl_state.entry_count--;
        tcl_state.stats.evictions++;
    }
    
    return TCL_STATUS_OK;
}

static tcl_status_t evict_fifo_entries(uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (tcl_state.entry_count == 0) {
            break;
        }

        uint32_t oldest_idx = 0;
        uint64_t oldest_timestamp = UINT64_MAX;
        
        for (uint32_t j = 0; j < tcl_state.entry_count; j++) {
            if (tcl_state.entries[j].timestamp < oldest_timestamp) {
                oldest_timestamp = tcl_state.entries[j].timestamp;
                oldest_idx = j;
            }
        }
        
        tcl_free_entry(&tcl_state.entries[oldest_idx]);
        
        if (oldest_idx < tcl_state.entry_count - 1) {
            memmove(&tcl_state.entries[oldest_idx],
                   &tcl_state.entries[tcl_state.entry_count - 1],
                   sizeof(tcl_entry_t));
        }
        
        tcl_state.entry_count--;
        tcl_state.stats.evictions++;
    }
    
    return TCL_STATUS_OK;
}

static tcl_status_t evict_random_entries(uint32_t count) {
    srand((unsigned int)time(NULL));
    
    for (uint32_t i = 0; i < count; i++) {
        if (tcl_state.entry_count == 0) {
            break;
        }
        
        uint32_t idx = rand() % tcl_state.entry_count;
        tcl_free_entry(&tcl_state.entries[idx]);
        
        if (idx < tcl_state.entry_count - 1) {
            memmove(&tcl_state.entries[idx],
                   &tcl_state.entries[tcl_state.entry_count - 1],
                   sizeof(tcl_entry_t));
        }
        
        tcl_state.entry_count--;
        tcl_state.stats.evictions++;
    }
    
    return TCL_STATUS_OK;
}

tcl_status_t tcl_entry_evict(uint32_t count) {
    if (!entry_manager_state.initialized) {
        tcl_set_last_error(TCL_STATUS_ERROR_NOT_INITIALIZED, 
                          "Entry manager not initialized");
        return TCL_STATUS_ERROR_NOT_INITIALIZED;
    }

    if (count == 0) {
        return TCL_STATUS_OK;
    }

    switch (entry_manager_state.config.policy) {
        case TCL_EVICT_LRU:
            return evict_lru_entries(count);
        case TCL_EVICT_LFU:
            return evict_lfu_entries(count);
        case TCL_EVICT_FIFO:
            return evict_fifo_entries(count);
        case TCL_EVICT_RANDOM:
            return evict_random_entries(count);
        default:
            tcl_set_last_error(TCL_STATUS_ERROR_INVALID_PARAM, 
                              "Invalid eviction policy");
            return TCL_STATUS_ERROR_INVALID_PARAM;
    }
}

tcl_status_t tcl_entry_manager_init(const tcl_entry_manager_config_t *config) {
    if (entry_manager_state.initialized) {
        tcl_set_last_error(TCL_STATUS_ERROR_ALREADY_INITIALIZED,
                          "Entry manager already initialized");
        return TCL_STATUS_ERROR_ALREADY_INITIALIZED;
    }

    if (config != NULL) {
        memcpy(&entry_manager_state.config, config, 
               sizeof(tcl_entry_manager_config_t));
    } else {
        // Use defaults
        entry_manager_state.config.policy = TCL_DEFAULT_EVICTION_POLICY;
        entry_manager_state.config.eviction_batch_size = TCL_DEFAULT_EVICTION_BATCH;
        entry_manager_state.config.min_free_entries = TCL_DEFAULT_MIN_FREE_ENTRIES;
        entry_manager_state.config.auto_extend_ttl = TCL_DEFAULT_AUTO_EXTEND_TTL;
        entry_manager_state.config.ttl_extension_ms = TCL_DEFAULT_TTL_EXTENSION_MS;
    }

    entry_manager_state.initialized = true;
    TCL_LOG("Entry manager initialized with policy=%d", 
            entry_manager_state.config.policy);
    return TCL_STATUS_OK;
}

tcl_status_t tcl_entry_clear_expired(void) {
    uint32_t initial_count = tcl_state.entry_count;
    uint64_t current_time = sys_get_time_ms();
    
    for (uint32_t i = 0; i < tcl_state.entry_count;) {
        if (current_time - tcl_state.entries[i].timestamp > 
            tcl_state.entries[i].ttl) {
            // Entry expired, remove it
            tcl_free_entry(&tcl_state.entries[i]);
            
            // Move last entry to this position if not already last
            if (i < tcl_state.entry_count - 1) {
                memmove(&tcl_state.entries[i],
                       &tcl_state.entries[tcl_state.entry_count - 1],
                       sizeof(tcl_entry_t));
            }
            
            tcl_state.entry_count--;
            tcl_state.stats.evictions++;
            // Don't increment i since we moved a new entry to this position
        } else {
            i++; // Only increment if we didn't remove the current entry
        }
    }
    
    uint32_t removed = initial_count - tcl_state.entry_count;
    TCL_LOG("Cleared %u expired entries", removed);
    return TCL_STATUS_OK;
}

tcl_status_t tcl_entry_extend_ttl(const char *key, uint32_t extension_ms) {
    TCL_RETURN_IF_NULL(key, "Key is NULL");
    
    tcl_entry_t *entry;
    TCL_RETURN_IF_ERROR(tcl_find_entry(key, &entry));
    
    entry->ttl += extension_ms;
    TCL_LOG("Extended TTL for key %s by %u ms", key, extension_ms);
    return TCL_STATUS_OK;
}

uint32_t tcl_entry_get_count(void) {
    return tcl_state.entry_count;
}

uint32_t tcl_entry_get_free_space(void) {
    return tcl_state.config.max_entries - tcl_state.entry_count;
}

float tcl_entry_get_usage_percent(void) {
    return (float)tcl_state.entry_count * 100.0f / 
           (float)tcl_state.config.max_entries;
}
