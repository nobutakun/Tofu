/**
 * @file tcl_entry_manager.h
 * @brief Cache entry management for Translation Cache Layer
 */

#ifndef TCL_ENTRY_MANAGER_H
#define TCL_ENTRY_MANAGER_H

#include "translation_cache_layer.h"
#include <stdint.h>
#include <stdbool.h>

// TTL and eviction policies
typedef enum {
    TCL_EVICT_LRU = 0,      // Least Recently Used
    TCL_EVICT_LFU = 1,      // Least Frequently Used
    TCL_EVICT_FIFO = 2,     // First In First Out
    TCL_EVICT_RANDOM = 3    // Random Selection
} tcl_eviction_policy_t;

// Entry manager configuration
typedef struct {
    tcl_eviction_policy_t policy;
    uint32_t eviction_batch_size;  // Number of entries to evict at once
    uint32_t min_free_entries;     // Minimum number of free entries to maintain
    bool auto_extend_ttl;          // Whether to extend TTL on access
    uint32_t ttl_extension_ms;     // How much to extend TTL by
} tcl_entry_manager_config_t;

// Default configuration values
#define TCL_DEFAULT_EVICTION_POLICY TCL_EVICT_LRU
#define TCL_DEFAULT_EVICTION_BATCH 10
#define TCL_DEFAULT_MIN_FREE_ENTRIES 50
#define TCL_DEFAULT_AUTO_EXTEND_TTL true
#define TCL_DEFAULT_TTL_EXTENSION_MS (6 * 60 * 60 * 1000) // 6 hours

// Public interface
tcl_status_t tcl_entry_manager_init(const tcl_entry_manager_config_t *config);
tcl_status_t tcl_entry_manager_deinit(void);

tcl_status_t tcl_entry_add(tcl_entry_t *entry);
tcl_status_t tcl_entry_remove(const char *key);
tcl_status_t tcl_entry_update(const char *key, const tcl_entry_t *new_data);
tcl_status_t tcl_entry_get(const char *key, tcl_entry_t *entry);

tcl_status_t tcl_entry_evict(uint32_t count);
tcl_status_t tcl_entry_clear_expired(void);
tcl_status_t tcl_entry_extend_ttl(const char *key, uint32_t extension_ms);

// Statistics and monitoring
uint32_t tcl_entry_get_count(void);
uint32_t tcl_entry_get_free_space(void);
float tcl_entry_get_usage_percent(void);

#endif // TCL_ENTRY_MANAGER_H
