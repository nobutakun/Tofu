/**
 * @file tcl_redis_schema.c
 * @brief Redis schema implementation
 */

#include "tcl_redis_schema.h"
#include "tcl_redis.h"
#include "tcl_state.h"
#include "../../system_manager.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

// Schema configuration
static struct {
    tcl_redis_persist_config_t config;
    bool initialized;
    uint32_t current_version;
} schema_state = {
    .initialized = false,
    .current_version = 0
};

tcl_status_t tcl_redis_schema_init(const tcl_redis_persist_config_t *config) {
    if (schema_state.initialized) {
        return TCL_STATUS_ERROR_ALREADY_INITIALIZED;
    }

    // Store configuration
    if (config != NULL) {
        memcpy(&schema_state.config, config, sizeof(tcl_redis_persist_config_t));
    } else {
        schema_state.config.enable_persistence = true;
        schema_state.config.rdb_filename = TCL_REDIS_DEFAULT_RDB_FILE;
        schema_state.config.save_interval_sec = TCL_REDIS_DEFAULT_SAVE_INTERVAL;
        schema_state.config.min_changes = TCL_REDIS_DEFAULT_MIN_CHANGES;
    }

    // Configure Redis persistence
    if (schema_state.config.enable_persistence) {
        tcl_redis_context_t *context;
        TCL_RETURN_IF_ERROR(tcl_redis_get_connection(&context));

        // Set save parameters
        char cmd_buf[256];
        snprintf(cmd_buf, sizeof(cmd_buf), 
                "CONFIG SET save \"%u %u\"",
                schema_state.config.save_interval_sec,
                schema_state.config.min_changes);
        
        tcl_status_t status = redis_send_command(context, "%s" REDIS_DELIM, cmd_buf);

        if (status == TCL_STATUS_OK) {
            snprintf(cmd_buf, sizeof(cmd_buf),
                    "CONFIG SET dbfilename \"%s\"",
                    schema_state.config.rdb_filename);
            status = redis_send_command(context, "%s" REDIS_DELIM, cmd_buf);
        }

        tcl_redis_return_connection(context);
        if (status != TCL_STATUS_OK) {
            return status;
        }
    }

    schema_state.initialized = true;
    return TCL_STATUS_OK;
}

tcl_status_t tcl_redis_schema_migrate(void) {
    tcl_redis_context_t *context;
    TCL_RETURN_IF_ERROR(tcl_redis_get_connection(&context));

    // Get current schema version
    uint32_t current_version = 0;
    char cmd_buf[256];
    snprintf(cmd_buf, sizeof(cmd_buf),
            "GET %sversion",
            TCL_REDIS_PREFIX_META);
    
    tcl_status_t status = redis_send_command(context, "%s" REDIS_DELIM, cmd_buf);
    
    if (status == TCL_STATUS_OK) {
        tcl_redis_reply_t *reply;
        status = redis_read_response(context, &reply);
        if (status == TCL_STATUS_OK) {
            if (reply->type == TCL_REDIS_REPLY_STRING) {
                current_version = atoi(reply->value.str.str);
            }
            tcl_redis_free_reply(reply);
        }
    }

    // Perform migrations if needed
    if (current_version < TCL_REDIS_SCHEMA_VERSION) {
        TCL_LOG("Migrating Redis schema from version %u to %u",
                current_version, TCL_REDIS_SCHEMA_VERSION);

        // Add your migration steps here for each version
        if (current_version < 1) {
            snprintf(cmd_buf, sizeof(cmd_buf),
                    "SADD %sschemas translation",
                    TCL_REDIS_PREFIX_META);
            status = redis_send_command(context, "%s" REDIS_DELIM, cmd_buf);
        }

        if (status == TCL_STATUS_OK) {
            snprintf(cmd_buf, sizeof(cmd_buf),
                    "SET %sversion %u",
                    TCL_REDIS_PREFIX_META,
                    TCL_REDIS_SCHEMA_VERSION);
            status = redis_send_command(context, "%s" REDIS_DELIM, cmd_buf);
        }
    }

    tcl_redis_return_connection(context);
    return status;
}

tcl_status_t tcl_redis_schema_backup(const char *backup_file) {
    if (!schema_state.config.enable_persistence) {
        return TCL_STATUS_ERROR_INVALID_PARAM;
    }

    tcl_redis_context_t *context;
    TCL_RETURN_IF_ERROR(tcl_redis_get_connection(&context));

    // Trigger Redis SAVE command
    tcl_status_t status = redis_send_command(context, "SAVE" REDIS_DELIM);
    tcl_redis_return_connection(context);

    if (status == TCL_STATUS_OK) {
        if (hal_file_copy(schema_state.config.rdb_filename, backup_file) != 0) {
            status = TCL_STATUS_ERROR_INTERNAL;
        }
    }

    return status;
}

tcl_status_t tcl_redis_schema_restore(const char *backup_file) {
    if (!schema_state.config.enable_persistence) {
        return TCL_STATUS_ERROR_INVALID_PARAM;
    }

    // Stop Redis connections
    tcl_redis_deinit();

    // Replace RDB file with backup
    if (hal_file_copy(backup_file, schema_state.config.rdb_filename) != 0) {
        return TCL_STATUS_ERROR_INTERNAL;
    }

    // Restart Redis connections
    tcl_redis_config_t config = {0};
    return tcl_redis_init(&config);
}

tcl_status_t tcl_redis_validate_schema(void) {
    tcl_redis_context_t *context;
    TCL_RETURN_IF_ERROR(tcl_redis_get_connection(&context));

    // Check required keys exist
    char cmd_buf[256];
    snprintf(cmd_buf, sizeof(cmd_buf),
            "EXISTS %sversion %sschemas",
            TCL_REDIS_PREFIX_META,
            TCL_REDIS_PREFIX_META);
    
    tcl_status_t status = redis_send_command(context, "%s" REDIS_DELIM, cmd_buf);

    tcl_redis_reply_t *reply;
    if (status == TCL_STATUS_OK) {
        status = redis_read_response(context, &reply);
        if (status == TCL_STATUS_OK) {
            if (reply->type != TCL_REDIS_REPLY_INTEGER ||
                reply->value.integer != 2) {
                status = TCL_STATUS_ERROR_INVALID_PARAM;
            }
            tcl_redis_free_reply(reply);
        }
    }

    tcl_redis_return_connection(context);
    return status;
}

uint32_t tcl_redis_get_schema_version(void) {
    return schema_state.current_version;
}
