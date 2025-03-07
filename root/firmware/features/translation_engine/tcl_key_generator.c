/**
 * @file tcl_key_generator.c
 * @brief Implementation of cache key generation
 */

#include "tcl_key_generator.h"
#include "tcl_state.h"
#include "../../system_manager.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// Define the maximum length for the normalized text
#define TCL_KEY_MAX_LENGTH 256

// Internal state
static struct {
    tcl_key_config_t config;
    bool initialized;
} key_gen_state = {
    .initialized = false
};

// FNV-1a hashing constants
#define FNV_PRIME 16777619
#define FNV_OFFSET_BASIS 2166136261u

// Forward declarations
static uint32_t generate_fnv1a_hash(const char *text, size_t length);
static void normalize_text(const char *input, char *output, size_t output_size);

/**
 * @brief Initialize key generator with configuration
 */
tcl_status_t tcl_key_init(const tcl_key_config_t *config) {
    if (key_gen_state.initialized) {
        tcl_set_last_error(TCL_STATUS_ERROR_ALREADY_INITIALIZED, 
                          "Key generator already initialized");
        return TCL_STATUS_ERROR_ALREADY_INITIALIZED;
    }

    if (config == NULL) {
        // Use default configuration
        key_gen_state.config.method = TCL_KEY_DEFAULT_METHOD;
        key_gen_state.config.seed = TCL_KEY_DEFAULT_SEED;
        key_gen_state.config.normalize_text = TCL_KEY_NORMALIZE_TEXT;
        key_gen_state.config.include_timestamp = TCL_KEY_INCLUDE_TIMESTAMP;
    } else {
        memcpy(&key_gen_state.config, config, sizeof(tcl_key_config_t));
    }

    key_gen_state.initialized = true;
    TCL_LOG("Key generator initialized with method=%d, seed=0x%x", 
            key_gen_state.config.method, key_gen_state.config.seed);
    return TCL_STATUS_OK;
}

/**
 * @brief Generate FNV-1a hash of input text
 */
static uint32_t generate_fnv1a_hash(const char *text, size_t length) {
    uint32_t hash = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < length; i++) {
        hash ^= (uint8_t)text[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

/**
 * @brief Normalize text by removing whitespace and converting to lowercase
 */
static void normalize_text(const char *input, char *output, size_t output_size) {
    size_t j = 0;
    for (size_t i = 0; input[i] && j < output_size - 1; i++) {
        if (!isspace((unsigned char)input[i])) {
            output[j++] = tolower((unsigned char)input[i]);
        }
    }
    output[j] = '\0';
}

/**
 * @brief Generate cache key from input parameters
 */
tcl_status_t tcl_key_generate(const char *source_text,
                             const char *source_lang,
                             const char *target_lang,
                             char *key_buffer,
                             size_t buffer_size) {
    if (!key_gen_state.initialized) {
        tcl_set_last_error(TCL_STATUS_ERROR_NOT_INITIALIZED, 
                          "Key generator not initialized");
        return TCL_STATUS_ERROR_NOT_INITIALIZED;
    }

    if (source_text == NULL || source_lang == NULL || 
        target_lang == NULL || key_buffer == NULL) {
        tcl_set_last_error(TCL_STATUS_ERROR_INVALID_PARAM, 
                          "Invalid parameters provided");
        return TCL_STATUS_ERROR_INVALID_PARAM;
    }

    // Normalize text if configured
    char normalized_text[TCL_KEY_MAX_LENGTH];
    const char *text_to_hash = source_text;
    
    if (key_gen_state.config.normalize_text) {
        normalize_text(source_text, normalized_text, sizeof(normalized_text));
        text_to_hash = normalized_text;
    }

    // Generate hash based on selected method
    uint32_t hash;
    switch (key_gen_state.config.method) {
        case TCL_KEY_METHOD_FNV1A:
            hash = generate_fnv1a_hash(text_to_hash, strlen(text_to_hash));
            break;
            
        case TCL_KEY_METHOD_MURMUR3:
        case TCL_KEY_METHOD_CUSTOM:
            tcl_set_last_error(TCL_STATUS_ERROR_NOT_IMPLEMENTED, 
                             "Hash method not implemented");
            return TCL_STATUS_ERROR_NOT_IMPLEMENTED;
            
        default:
            tcl_set_last_error(TCL_STATUS_ERROR_INVALID_PARAM, 
                             "Invalid hash method");
            return TCL_STATUS_ERROR_INVALID_PARAM;
    }

    // Format key: source_lang:target_lang:hash[:timestamp]
    int written;
    if (key_gen_state.config.include_timestamp) {
        uint64_t timestamp = tcl_get_time_ms();
        written = snprintf(key_buffer, buffer_size, "%s:%s:%08x:%lu",
                         source_lang, target_lang, hash, timestamp);
    } else {
        written = snprintf(key_buffer, buffer_size, "%s:%s:%08x",
                         source_lang, target_lang, hash);
    }

    if (written < 0 || (size_t)written >= buffer_size) {
        tcl_set_last_error(TCL_STATUS_ERROR_INVALID_PARAM, 
                          "Key buffer too small");
        return TCL_STATUS_ERROR_INVALID_PARAM;
    }

    TCL_LOG("Generated key: %s", key_buffer);
    return TCL_STATUS_OK;
}

/**
 * @brief Set the key generation method
 */
void tcl_key_set_method(tcl_key_method_t method) {
    if (key_gen_state.initialized) {
        key_gen_state.config.method = method;
        TCL_LOG("Key generation method changed to %d", method);
    }
}

/**
 * @brief Get the current key generation method
 */
tcl_key_method_t tcl_key_get_method(void) {
    return key_gen_state.config.method;
}
