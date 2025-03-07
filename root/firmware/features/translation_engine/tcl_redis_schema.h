/**
 * @file tcl_redis_schema.h
 * @brief Redis schema definitions and serialization functions
 */

#ifndef TCL_REDIS_SCHEMA_H
#define TCL_REDIS_SCHEMA_H

#include "tcl_redis_types.h"
#include "translation_cache_layer.h"
#include <stdbool.h>

// Redis schema version
#define TCL_REDIS_SCHEMA_VERSION 1

// Schema field separators
#define TCL_REDIS_FIELD_SEPARATOR "|"
#define TCL_REDIS_METADATA_SEPARATOR ";"

// Entry serialization and parsing
char *tcl_redis_serialize_entry(const tcl_entry_t *entry);
tcl_status_t tcl_redis_parse_entry(const tcl_redis_reply_t *reply, tcl_entry_t *entry);

// Key formatting
tcl_status_t tcl_redis_format_key(const char *key, char *buffer, size_t buffer_size);

#endif // TCL_REDIS_SCHEMA_H
