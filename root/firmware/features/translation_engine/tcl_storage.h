/**
 * @file tcl_storage.h
 * @brief Persistent storage integration for Translation Cache Layer
 */

#ifndef TCL_STORAGE_H
#define TCL_STORAGE_H

#include "translation_cache_layer.h"
#include "tcl_redis_schema.h"
#include <stdint.h>
#include <stdbool.h>

// Storage configuration
typedef struct {
    bool enable_auto_save;       // Whether to automatically save changes
    bool enable_compression;     // Whether to compress data
    uint32_t auto_save_interval; // Interval between auto-saves (ms)
    uint32_t max_batch_size;     // Maximum entries to process in one batch
    const char *storage_path;    // Path to storage directory
} tcl_storage_config_t;

// Storage statistics
typedef struct {
    uint64_t total_saves;       // Total number of save operations
    uint64_t total_loads;       // Total number of load operations
    uint64_t failed_operations; // Number of failed operations
    uint64_t bytes_written;     // Total bytes written
    uint64_t bytes_read;        // Total bytes read
    uint64_t last_save_time;    // Timestamp of last save
    uint64_t last_load_time;    // Timestamp of last load
} tcl_storage_stats_t;

// Default configuration
#define TCL_STORAGE_DEFAULT_AUTO_SAVE_INTERVAL (15 * 60 * 1000) // 15 minutes
#define TCL_STORAGE_DEFAULT_MAX_BATCH 1000
#define TCL_STORAGE_DEFAULT_PATH "./tcl_storage"

// Public interface
tcl_status_t tcl_storage_init(const tcl_storage_config_t *config);
tcl_status_t tcl_storage_deinit(void);

// Core operations
tcl_status_t tcl_storage_save_all(void);
tcl_status_t tcl_storage_load_all(void);
tcl_status_t tcl_storage_clear_all(void);

// Batch operations
tcl_status_t tcl_storage_save_batch(const tcl_entry_t *entries, uint32_t count);
tcl_status_t tcl_storage_load_batch(uint32_t offset, uint32_t count, 
                                  tcl_entry_t *entries, uint32_t *loaded);

// Utility functions
tcl_status_t tcl_storage_get_stats(tcl_storage_stats_t *stats);
tcl_status_t tcl_storage_verify_integrity(void);
bool tcl_storage_needs_save(void);

#endif // TCL_STORAGE_H
