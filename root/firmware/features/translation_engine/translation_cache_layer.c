/**
 * @file translation_cache_layer.c
 * @brief Translation Cache Layer (TCL) implementation - Phase 1
 */

#include "translation_cache_layer.h"
#include "tcl_state.h"
#include "../../system_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/types.h>

// Core cache functions
static void tcl_free_entry(tcl_entry_t *entry) {
    if (entry == NULL) {
        return;
    }
    free(entry->source_text);
    free(entry->source_lang);
    free(entry->target_lang);
    free(entry->translation);
    if (entry->metadata.context != NULL) {
        free(entry->metadata.context);
    }
    memset(entry, 0, sizeof(tcl_entry_t));
}

static tcl_status_t tcl_init_memory_cache(void) {
    tcl_state.entries = calloc(tcl_state.config.max_entries, sizeof(tcl_entry_t));
    if (tcl_state.entries == NULL) {
        tcl_set_last_error(TCL_STATUS_ERROR_MEMORY, "Failed to allocate memory cache");
        return TCL_STATUS_ERROR_MEMORY;
    }
    tcl_state.entry_count = 0;
    return TCL_STATUS_OK;
}

// Public API Implementation

tcl_status_t tcl_init(tcl_config_t *config) {
    if (tcl_state.initialized) {
        tcl_set_last_error(TCL_STATUS_ERROR_ALREADY_INITIALIZED, "Cache already initialized");
        return TCL_STATUS_ERROR_ALREADY_INITIALIZED;
    }

    TCL_RETURN_IF_NULL(config, "Configuration is NULL");
    
    // Initialize internal state
    tcl_state_init();
    memcpy(&tcl_state.config, config, sizeof(tcl_config_t));
    
    // Set default values if not provided
    if (tcl_state.config.max_entries == 0) {
        tcl_state.config.max_entries = TCL_DEFAULT_MAX_ENTRIES;
    }
    if (tcl_state.config.default_ttl_ms == 0) {
        tcl_state.config.default_ttl_ms = TCL_DEFAULT_TTL_MS;
    }
    
    TCL_RETURN_IF_ERROR(tcl_init_memory_cache());
    
    tcl_state.initialized = true;
    TCL_LOG("Cache initialized with max_entries=%u, default_ttl=%u",
            tcl_state.config.max_entries, tcl_state.config.default_ttl_ms);
    return TCL_STATUS_OK;
}

tcl_status_t tcl_deinit(void) {
    TCL_RETURN_IF_ERROR(tcl_validate_init());
    
    for (uint32_t i = 0; i < tcl_state.entry_count; i++) {
        tcl_free_entry(&tcl_state.entries[i]);
    }
    
    free(tcl_state.entries);
    tcl_state.entries = NULL;
    tcl_state.entry_count = 0;
    tcl_state.initialized = false;
    
    TCL_LOG("Cache deinitialized");
    return TCL_STATUS_OK;
}

tcl_status_t tcl_get(const char *source_text, 
                     const char *source_lang,
                     const char *target_lang,
                     tcl_entry_t *entry) {
    TCL_RETURN_IF_ERROR(tcl_validate_init());
    TCL_RETURN_IF_ERROR(tcl_validate_params_basic(source_text, source_lang, target_lang));
    TCL_RETURN_IF_NULL(entry, "Output entry is NULL");
    
    char key[TCL_KEY_MAX_LENGTH];
    TCL_RETURN_IF_ERROR(tcl_generate_key(source_text, source_lang, target_lang, 
                                       key, sizeof(key)));
    
    uint64_t start_time = tcl_get_time_ms();
    tcl_entry_t *cached_entry;
    tcl_status_t status = tcl_find_entry(key, &cached_entry);
    
    if (status == TCL_STATUS_OK) {
        if (tcl_get_time_ms() - cached_entry->timestamp > cached_entry->ttl) {
            TCL_LOG("Entry found but expired for key: %s", key);
            tcl_free_entry(cached_entry);
            tcl_state_update_stats(false, tcl_get_time_ms() - start_time);
            return TCL_STATUS_ERROR_NOT_FOUND;
        }
        
        status = tcl_copy_entry(cached_entry, entry);
        if (status != TCL_STATUS_OK) {
            return status;
        }
        
        cached_entry->metadata.usage_count++;
        cached_entry->metadata.last_used = tcl_get_time_ms();
        tcl_state_update_stats(true, tcl_get_time_ms() - start_time);
        return TCL_STATUS_OK;
    }
    
    tcl_state_update_stats(false, tcl_get_time_ms() - start_time);
    return TCL_STATUS_ERROR_NOT_FOUND;
}

tcl_status_t tcl_set(const char *source_text,
                     const char *source_lang,
                     const char *target_lang,
                     const char *translation,
                     const tcl_metadata_t *metadata,
                     uint32_t ttl) {
    TCL_RETURN_IF_ERROR(tcl_validate_init());
    TCL_RETURN_IF_ERROR(tcl_validate_params_basic(source_text, source_lang, target_lang));
    TCL_RETURN_IF_NULL(translation, "Translation text is NULL");
    
    // Handle cache full condition
    if (tcl_state.entry_count >= tcl_state.config.max_entries) {
        TCL_RETURN_IF_ERROR(tcl_evict_entries(1));
    }
    
    // Create new entry
    tcl_entry_t *new_entry = &tcl_state.entries[tcl_state.entry_count];
    memset(new_entry, 0, sizeof(tcl_entry_t));
    
    new_entry->source_text = strdup(source_text);
    new_entry->source_lang = strdup(source_lang);
    new_entry->target_lang = strdup(target_lang);
    new_entry->translation = strdup(translation);
    
    if (!new_entry->source_text || !new_entry->source_lang || 
        !new_entry->target_lang || !new_entry->translation) {
        tcl_free_entry(new_entry);
        tcl_set_last_error(TCL_STATUS_ERROR_MEMORY, "Failed to allocate entry strings");
        return TCL_STATUS_ERROR_MEMORY;
    }
    
    // Set metadata
    if (metadata) {
        new_entry->metadata = *metadata;
        if (metadata->context) {
            new_entry->metadata.context = strdup(metadata->context);
            if (!new_entry->metadata.context) {
                tcl_free_entry(new_entry);
                return TCL_STATUS_ERROR_MEMORY;
            }
        }
    } else {
        new_entry->metadata.usage_count = 1;
        new_entry->metadata.last_used = tcl_get_time_ms();
        new_entry->metadata.context = NULL;
    }
    
    new_entry->timestamp = tcl_get_time_ms();
    new_entry->ttl = ttl ? ttl : tcl_state.config.default_ttl_ms;
    
    tcl_state.entry_count++;
    TCL_LOG("Added new cache entry, total entries: %u", tcl_state.entry_count);
    return TCL_STATUS_OK;
}

tcl_status_t tcl_exists(const char *source_text,
                       const char *source_lang,
                       const char *target_lang,
                       bool *exists) {
    TCL_RETURN_IF_ERROR(tcl_validate_init());
    TCL_RETURN_IF_ERROR(tcl_validate_params_basic(source_text, source_lang, target_lang));
    TCL_RETURN_IF_NULL(exists, "Output exists flag is NULL");
    
    char key[TCL_KEY_MAX_LENGTH];
    TCL_RETURN_IF_ERROR(tcl_generate_key(source_text, source_lang, target_lang, 
                                       key, sizeof(key)));
    
    tcl_entry_t *entry;
    tcl_status_t status = tcl_find_entry(key, &entry);
    
    if (status == TCL_STATUS_OK) {
        *exists = (tcl_get_time_ms() - entry->timestamp <= entry->ttl);
    } else {
        *exists = false;
    }
    
    return TCL_STATUS_OK;
}

tcl_status_t tcl_get_stats(tcl_stats_t *stats) {
    TCL_RETURN_IF_ERROR(tcl_validate_init());
    TCL_RETURN_IF_NULL(stats, "Output stats is NULL");
    
    memcpy(stats, &tcl_state.stats, sizeof(tcl_stats_t));
    stats->current_entries = tcl_state.entry_count;
    
    return TCL_STATUS_OK;
}

tcl_status_t tcl_get_memory_usage(uint32_t *usage_kb) {
    TCL_RETURN_IF_ERROR(tcl_validate_init());
    TCL_RETURN_IF_NULL(usage_kb, "Output usage_kb is NULL");
    
    uint32_t total_bytes = sizeof(tcl_state_t);
    total_bytes += tcl_state.config.max_entries * sizeof(tcl_entry_t);
    
    for (uint32_t i = 0; i < tcl_state.entry_count; i++) {
        if (tcl_state.entries[i].source_text) {
            total_bytes += strlen(tcl_state.entries[i].source_text) + 1;
        }
        if (tcl_state.entries[i].source_lang) {
            total_bytes += strlen(tcl_state.entries[i].source_lang) + 1;
        }
        if (tcl_state.entries[i].target_lang) {
            total_bytes += strlen(tcl_state.entries[i].target_lang) + 1;
        }
        if (tcl_state.entries[i].translation) {
            total_bytes += strlen(tcl_state.entries[i].translation) + 1;
        }
        if (tcl_state.entries[i].metadata.context) {
            total_bytes += strlen(tcl_state.entries[i].metadata.context) + 1;
        }
    }
    
    *usage_kb = TCL_ALIGN_KB(total_bytes);
    return TCL_STATUS_OK;
}
