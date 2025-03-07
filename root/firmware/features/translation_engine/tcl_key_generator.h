/**
 * @file tcl_key_generator.h
 * @brief Cache key generation functionality for Translation Cache Layer
 */

#ifndef TCL_KEY_GENERATOR_H
#define TCL_KEY_GENERATOR_H

#include "translation_cache_layer.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Key size limitations
#define TCL_KEY_MAX_LENGTH 256

// Key generation methods
typedef enum {
    TCL_KEY_METHOD_FNV1A = 0,    // FNV-1a hashing
    TCL_KEY_METHOD_MURMUR3 = 1,  // MurmurHash3
    TCL_KEY_METHOD_CUSTOM = 2     // Custom hashing
} tcl_key_method_t;

// Key generation configuration
typedef struct {
    tcl_key_method_t method;      // Hashing method to use
    uint32_t seed;                // Hash seed value
    bool normalize_text;          // Whether to normalize text before hashing
    bool include_timestamp;       // Whether to include timestamp in key
} tcl_key_config_t;

// Default configuration values
#define TCL_KEY_DEFAULT_METHOD TCL_KEY_METHOD_FNV1A
#define TCL_KEY_DEFAULT_SEED 0x1234ABCD
#define TCL_KEY_NORMALIZE_TEXT true
#define TCL_KEY_INCLUDE_TIMESTAMP false

// Public interface
tcl_status_t tcl_key_init(const tcl_key_config_t *config);
tcl_status_t tcl_key_generate(const char *source_text,
                             const char *source_lang,
                             const char *target_lang,
                             char *key_buffer,
                             size_t buffer_size);
void tcl_key_set_method(tcl_key_method_t method);
tcl_key_method_t tcl_key_get_method(void);

#endif // TCL_KEY_GENERATOR_H
