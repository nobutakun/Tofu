/**
 * @file tcl_storage.c
 * @brief Implementation of persistent storage integration
 */

#include "tcl_storage.h"
#include "tcl_state.h"
#include "../../system_manager.h"
#include "../../hal.h"

#ifndef HAL_SEEK_CUR
#define HAL_SEEK_CUR 1
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Storage state
static struct {
    tcl_storage_config_t config;
    tcl_storage_stats_t stats;
    uint32_t pending_changes;
    bool initialized;
    uint64_t last_auto_save;
} storage_state = {
    .initialized = false,
    .pending_changes = 0,
    .last_auto_save = 0
};

// File paths
#define METADATA_FILE "metadata.bin"
#define ENTRIES_FILE "entries.bin"
#define INDEX_FILE "index.bin"
#define TEMP_SUFFIX ".tmp"

// Internal helper functions
static tcl_status_t ensure_storage_directory(void) {
    if (!hal_dir_exists(storage_state.config.storage_path)) {
        if (hal_dir_create(storage_state.config.storage_path) != 0) {
            tcl_set_last_error(TCL_STATUS_ERROR_STORAGE,
                             "Failed to create storage directory");
            return TCL_STATUS_ERROR_STORAGE;
        }
    }
    return TCL_STATUS_OK;
}

static char* get_full_path(const char *filename) {
    size_t path_len = strlen(storage_state.config.storage_path);
    size_t file_len = strlen(filename);
    char *full_path = malloc(path_len + file_len + 2); // +2 for / and \0
    
    if (full_path) {
        sprintf(full_path, "%s/%s", storage_state.config.storage_path, filename);
    }
    return full_path;
}

tcl_status_t tcl_storage_init(const tcl_storage_config_t *config) {
    if (storage_state.initialized) {
        return TCL_STATUS_ERROR_ALREADY_INITIALIZED;
    }

    // Store configuration
    if (config != NULL) {
        memcpy(&storage_state.config, config, sizeof(tcl_storage_config_t));
    } else {
        storage_state.config.enable_auto_save = true;
        storage_state.config.enable_compression = true;
        storage_state.config.auto_save_interval = TCL_STORAGE_DEFAULT_AUTO_SAVE_INTERVAL;
        storage_state.config.max_batch_size = TCL_STORAGE_DEFAULT_MAX_BATCH;
        storage_state.config.storage_path = TCL_STORAGE_DEFAULT_PATH;
    }

    // Initialize storage directory
    TCL_RETURN_IF_ERROR(ensure_storage_directory());

    // Try to load existing metadata
    if (read_metadata() != TCL_STATUS_OK) {
        // Initialize new stats if no existing metadata
        memset(&storage_state.stats, 0, sizeof(tcl_storage_stats_t));
    }

    storage_state.initialized = true;
    storage_state.last_auto_save = hal_get_time_ms();

    sys_log("TCL", "Storage initialized at %s", storage_state.config.storage_path);
    return TCL_STATUS_OK;
}

tcl_status_t tcl_storage_save_all(void) {
    if (!storage_state.initialized) {
        return TCL_STATUS_ERROR_NOT_INITIALIZED;
    }

    // First save Redis data
    TCL_RETURN_IF_ERROR(tcl_redis_schema_backup(
        storage_state.config.storage_path));

    // Save metadata
    TCL_RETURN_IF_ERROR(write_metadata());

    storage_state.stats.total_saves++;
    storage_state.stats.last_save_time = hal_get_time_ms();
    storage_state.last_auto_save = storage_state.stats.last_save_time;
    storage_state.pending_changes = 0;

    sys_log("TCL", "Storage state saved successfully");
    return TCL_STATUS_OK;
}

tcl_status_t tcl_storage_save_batch(const tcl_entry_t *entries, uint32_t count) {
    if (!storage_state.initialized) {
        return TCL_STATUS_ERROR_NOT_INITIALIZED;
    }

    // Validate parameters
    if (!entries || count == 0) {
        return TCL_STATUS_ERROR_INVALID_PARAM;
    }

    // Create batch file path
    char batch_path[256];
    snprintf(batch_path, sizeof(batch_path), 
             "%s/batch_%lu.bin",
             storage_state.config.storage_path,
             (unsigned long)hal_get_time_ms());

    FILE *f;
    if (hal_file_open(batch_path, "wb", &f) != HAL_FS_OK) {
        return TCL_STATUS_ERROR_STORAGE;
    }

    // Write batch header
    uint32_t magic = 0x54434C42; // "TCLB"
    uint32_t version = 1;
    size_t written;
    if (hal_file_write(f, &magic, sizeof(magic), 1, &written) != HAL_FS_OK ||
        hal_file_write(f, &version, sizeof(version), 1, &written) != HAL_FS_OK ||
        hal_file_write(f, &count, sizeof(count), 1, &written) != HAL_FS_OK) {
        hal_file_close(f);
        return TCL_STATUS_ERROR_STORAGE;
    }

    // Write entries
    for (uint32_t i = 0; i < count; i++) {
        const tcl_entry_t *entry = &entries[i];
        
        // Write entry data
        uint32_t key_len = strlen(entry->key);
        uint32_t value_len = strlen(entry->value);

        if (hal_file_write(f, &key_len, sizeof(key_len), 1, &written) != HAL_FS_OK ||
            hal_file_write(f, &value_len, sizeof(value_len), 1, &written) != HAL_FS_OK ||
            hal_file_write(f, entry->key, 1, key_len, &written) != HAL_FS_OK ||
            hal_file_write(f, entry->value, 1, value_len, &written) != HAL_FS_OK ||
            hal_file_write(f, &entry->timestamp, sizeof(entry->timestamp), 1, &written) != HAL_FS_OK ||
            hal_file_write(f, &entry->ttl, sizeof(entry->ttl), 1, &written) != HAL_FS_OK ||
            hal_file_write(f, &entry->flags, sizeof(entry->flags), 1, &written) != HAL_FS_OK) {
            hal_file_close(f);
            return TCL_STATUS_ERROR_STORAGE;
        }
    }

    hal_file_close(f);
    storage_state.stats.total_saves++;
    storage_state.pending_changes += count;

    sys_log("TCL", "Saved %u entries to batch file %s", count, batch_path);
    return TCL_STATUS_OK;
}

tcl_status_t tcl_storage_load_batch(uint32_t offset, uint32_t count,
                                  tcl_entry_t *entries, uint32_t *loaded) {
    if (!storage_state.initialized) {
        return TCL_STATUS_ERROR_NOT_INITIALIZED;
    }

    if (!entries || !loaded) {
        return TCL_STATUS_ERROR_INVALID_PARAM;
    }

    *loaded = 0;

    // Find newest batch file
    char **dir_entries;
    size_t dir_count;
    if (hal_list_dir(storage_state.config.storage_path, 
                     &dir_entries, &dir_count) != HAL_FS_OK) {
        return TCL_STATUS_ERROR_STORAGE;
    }

    // Process batch file
    char *batch_file = NULL;
    uint64_t newest_time = 0;

    for (size_t i = 0; i < dir_count; i++) {
        if (strncmp(dir_entries[i], "batch_", 6) == 0) {
            uint64_t timestamp;
            if (sscanf(dir_entries[i] + 6, "%lu", &timestamp) == 1) {
                if (timestamp > newest_time) {
                    newest_time = timestamp;
                    batch_file = dir_entries[i];
                }
            }
        }
    }

    if (!batch_file) {
        hal_free_dir_list(dir_entries, dir_count);
        return TCL_STATUS_ERROR_NOT_FOUND;
    }

    char batch_path[256];
    snprintf(batch_path, sizeof(batch_path), "%s/%s",
             storage_state.config.storage_path, batch_file);
    
    FILE *f;
    if (hal_file_open(batch_path, "rb", &f) != HAL_FS_OK) {
        hal_free_dir_list(dir_entries, dir_count);
        return TCL_STATUS_ERROR_STORAGE;
    }

    // Read and verify header
    uint32_t magic, version, total_count;
    size_t read_count;
    
    if (hal_file_read(f, &magic, sizeof(magic), 1, &read_count) != HAL_FS_OK ||
        hal_file_read(f, &version, sizeof(version), 1, &read_count) != HAL_FS_OK ||
        hal_file_read(f, &total_count, sizeof(total_count), 1, &read_count) != HAL_FS_OK ||
        magic != 0x54434C42 || version != 1) {
        hal_file_close(f);
        hal_free_dir_list(dir_entries, dir_count);
        return TCL_STATUS_ERROR_INVALID_FORMAT;
    }

    // Skip to offset
    for (uint32_t i = 0; i < offset && i < total_count; i++) {
        uint32_t key_len, value_len;
        if (hal_file_read(f, &key_len, sizeof(key_len), 1, &read_count) != HAL_FS_OK ||
            hal_file_read(f, &value_len, sizeof(value_len), 1, &read_count) != HAL_FS_OK) {
            hal_file_close(f);
            hal_free_dir_list(dir_entries, dir_count);
            return TCL_STATUS_ERROR_STORAGE;
        }
        
        // Skip entry data
        if (hal_file_seek(f, key_len + value_len + 
            sizeof(uint64_t) + sizeof(uint32_t) * 2, HAL_SEEK_CUR) != HAL_FS_OK) {
            hal_file_close(f);
            hal_free_dir_list(dir_entries, dir_count);
            return TCL_STATUS_ERROR_STORAGE;
        }
    }

    // Read requested entries
    uint32_t num_loaded = 0;
    for (uint32_t i = 0; i < count && (offset + i) < total_count; i++) {
        tcl_entry_t *entry = &entries[i];
        uint32_t key_len, value_len;

        if (hal_file_read(f, &key_len, sizeof(key_len), 1, &read_count) != HAL_FS_OK ||
            hal_file_read(f, &value_len, sizeof(value_len), 1, &read_count) != HAL_FS_OK) {
            break;
        }

        // Allocate and read strings
        entry->key = malloc(key_len + 1);
        entry->value = malloc(value_len + 1);
        if (!entry->key || !entry->value) {
            free(entry->key);
            free(entry->value);
            break;
        }

        if (hal_file_read(f, entry->key, 1, key_len, &read_count) != HAL_FS_OK ||
            hal_file_read(f, entry->value, 1, value_len, &read_count) != HAL_FS_OK) {
            free(entry->key);
            free(entry->value);
            break;
        }
        entry->key[key_len] = '\0';
        entry->value[value_len] = '\0';

        // Read remaining fields
        if (hal_file_read(f, &entry->timestamp, sizeof(entry->timestamp), 1, &read_count) != HAL_FS_OK ||
            hal_file_read(f, &entry->ttl, sizeof(entry->ttl), 1, &read_count) != HAL_FS_OK ||
            hal_file_read(f, &entry->flags, sizeof(entry->flags), 1, &read_count) != HAL_FS_OK) {
            free(entry->key);
            free(entry->value);
            break;
        }

        num_loaded++;
    }

    hal_file_close(f);
    hal_free_dir_list(dir_entries, dir_count);
    *loaded = num_loaded;
    storage_state.stats.total_loads++;

    sys_log("TCL", "Loaded %u entries from batch file %s", num_loaded, batch_path);
    return num_loaded > 0 ? TCL_STATUS_OK : TCL_STATUS_ERROR_EMPTY;
}

tcl_status_t tcl_storage_clear_all(void) {
    if (!storage_state.initialized) {
        return TCL_STATUS_ERROR_NOT_INITIALIZED;
    }

    char *files[] = {METADATA_FILE, ENTRIES_FILE, INDEX_FILE};
    for (size_t i = 0; i < sizeof(files)/sizeof(files[0]); i++) {
        char *path = get_full_path(files[i]);
        if (path) {
            hal_file_delete(path);
            free(path);
        }
    }

    memset(&storage_state.stats, 0, sizeof(tcl_storage_stats_t));
    storage_state.pending_changes = 0;

    sys_log("TCL", "Storage cleared successfully");
    return TCL_STATUS_OK;
}

tcl_status_t tcl_storage_deinit(void) {
    if (!storage_state.initialized) {
        return TCL_STATUS_ERROR_NOT_INITIALIZED;
    }

    // Save any pending changes
    if (storage_state.pending_changes > 0) {
        TCL_RETURN_IF_ERROR(tcl_storage_save_all());
    }

    storage_state.initialized = false;
    sys_log("TCL", "Storage deinitialized successfully");
    return TCL_STATUS_OK;
}
