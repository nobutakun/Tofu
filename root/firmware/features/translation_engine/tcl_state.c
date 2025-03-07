/**
 * @file tcl_state.c
 * @brief Implementation of TCL state management
 */

#include "tcl_state.h"
#include <string.h>
#include <stdio.h>

// Global state instance definition
tcl_state_t tcl_state = {
    .initialized = false,
    .entries = NULL,
    .entry_count = 0,
    .redis_ctx = NULL,
    .stats = {0},
    .config = {0}
};

// Error handling storage
static struct {
    tcl_status_t status;
    char message[256];
    bool has_error;
} tcl_last_error = {
    .status = TCL_STATUS_OK,
    .message = {0},
    .has_error = false
};

void tcl_state_init(void) {
    memset(&tcl_state, 0, sizeof(tcl_state_t));
    memset(&tcl_last_error, 0, sizeof(tcl_last_error));
    tcl_last_error.status = TCL_STATUS_OK;
    tcl_last_error.has_error = false;
}

void tcl_state_reset(void) {
    // Preserve configuration but reset everything else
    tcl_config_t saved_config = tcl_state.config;
    memset(&tcl_state, 0, sizeof(tcl_state_t));
    tcl_state.config = saved_config;
    
    // Reset error state
    memset(&tcl_last_error, 0, sizeof(tcl_last_error));
    tcl_last_error.status = TCL_STATUS_OK;
    tcl_last_error.has_error = false;
}

void tcl_state_update_stats(bool is_hit, uint64_t operation_time) {
    if (is_hit) {
        tcl_state.stats.hits++;
        if (tcl_state.stats.hits > 1) {
            tcl_state.stats.avg_hit_time_ms = 
                (tcl_state.stats.avg_hit_time_ms * (tcl_state.stats.hits - 1) + 
                 operation_time) / tcl_state.stats.hits;
        } else {
            tcl_state.stats.avg_hit_time_ms = operation_time;
        }
    } else {
        tcl_state.stats.misses++;
        if (tcl_state.stats.misses > 1) {
            tcl_state.stats.avg_miss_time_ms = 
                (tcl_state.stats.avg_miss_time_ms * (tcl_state.stats.misses - 1) + 
                 operation_time) / tcl_state.stats.misses;
        } else {
            tcl_state.stats.avg_miss_time_ms = operation_time;
        }
    }
}

tcl_status_t tcl_state_validate(void) {
    if (!tcl_state.initialized) {
        tcl_set_last_error(TCL_STATUS_ERROR_NOT_INITIALIZED, "Cache not initialized");
        return TCL_STATUS_ERROR_NOT_INITIALIZED;
    }
    
    if (tcl_state.entries == NULL) {
        tcl_set_last_error(TCL_STATUS_ERROR_INTERNAL, "Cache entries array is NULL");
        return TCL_STATUS_ERROR_INTERNAL;
    }
    
    if (tcl_state.entry_count > tcl_state.config.max_entries) {
        tcl_set_last_error(TCL_STATUS_ERROR_INTERNAL, "Entry count exceeds maximum");
        return TCL_STATUS_ERROR_INTERNAL;
    }
    
    return TCL_STATUS_OK;
}

tcl_status_t tcl_validate_init(void) {
    if (!tcl_state.initialized) {
        tcl_set_last_error(TCL_STATUS_ERROR_NOT_INITIALIZED, "Cache not initialized");
        return TCL_STATUS_ERROR_NOT_INITIALIZED;
    }
    return TCL_STATUS_OK;
}

tcl_status_t tcl_validate_params_basic(const char *source_text,
                                     const char *source_lang,
                                     const char *target_lang) {
    if (source_text == NULL || source_lang == NULL || target_lang == NULL) {
        tcl_set_last_error(TCL_STATUS_ERROR_INVALID_PARAM, "Required parameter is NULL");
        return TCL_STATUS_ERROR_INVALID_PARAM;
    }
    
    if (strlen(source_lang) == 0 || strlen(target_lang) == 0) {
        tcl_set_last_error(TCL_STATUS_ERROR_INVALID_PARAM, "Language code is empty");
        return TCL_STATUS_ERROR_INVALID_PARAM;
    }
    
    return TCL_STATUS_OK;
}

void tcl_log_operation(const char *op, tcl_status_t status) {
#ifdef TCL_DEBUG
    if (status == TCL_STATUS_OK) {
        printf("[TCL] %s: Success\n", op);
    } else {
        fprintf(stderr, "[TCL ERROR] %s: Failed with status %d - %s\n", 
                op, status, tcl_get_last_error());
    }
#endif
}

void tcl_set_last_error(tcl_status_t status, const char *message) {
    tcl_last_error.status = status;
    tcl_last_error.has_error = true;
    strncpy(tcl_last_error.message, message, sizeof(tcl_last_error.message) - 1);
    tcl_last_error.message[sizeof(tcl_last_error.message) - 1] = '\0';
    
    TCL_ERROR("%s", message);
}

const char *tcl_get_last_error(void) {
    return tcl_last_error.has_error ? tcl_last_error.message : "No error";
}
