/**
 * @file tcl_state.h
 * @brief Internal state management for Translation Cache Layer
 */

#ifndef TCL_STATE_H
#define TCL_STATE_H

#include "translation_cache_layer.h"
#include <stdbool.h>
#include <stdint.h>

// Internal state structure
typedef struct {
    bool initialized;
    tcl_config_t config;
    tcl_stats_t stats;
    tcl_entry_t *entries;
    uint32_t entry_count;
    void *redis_ctx;  // Will be cast to redisContext* when Redis is enabled
} tcl_state_t;

// Global state instance declaration
extern tcl_state_t tcl_state;

// State management functions
void tcl_state_init(void);
void tcl_state_reset(void);
void tcl_state_update_stats(bool is_hit, uint64_t operation_time);
tcl_status_t tcl_state_validate(void);

// Helper function declarations
tcl_status_t tcl_validate_init(void);
tcl_status_t tcl_validate_params_basic(const char *source_text,
                                     const char *source_lang,
                                     const char *target_lang);

// Internal error handling and logging
void tcl_log_operation(const char *op, tcl_status_t status);
void tcl_set_last_error(tcl_status_t status, const char *message);
const char *tcl_get_last_error(void);

#endif // TCL_STATE_H
