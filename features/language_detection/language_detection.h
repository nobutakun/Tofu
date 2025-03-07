/**
 * @file language_detection.h
 * @brief Language Detection (LD) module for To-fu device
 * 
 * This file defines the Language Detection module interface for the To-fu device.
 * The Language Detection module is responsible for identifying the language of
 * input text or audio using a two-tier approach: local detection on the device
 * and cloud-based detection when available. Optimized for offline-first operation
 * in Follower Bot role.
 */

#ifndef TOFU_LANGUAGE_DETECTION_H
#define TOFU_LANGUAGE_DETECTION_H

#include "../../firmware_config.h"
#include "../../hal.h"
#include "../../system_manager.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Language Detection status codes
 */
typedef enum {
    LD_STATUS_OK = 0,
    LD_STATUS_ERROR_GENERAL,
    LD_STATUS_ERROR_NOT_INITIALIZED,
    LD_STATUS_ERROR_ALREADY_INITIALIZED,
    LD_STATUS_ERROR_INVALID_PARAM,
    LD_STATUS_ERROR_NOT_SUPPORTED,
    LD_STATUS_ERROR_RESOURCE_BUSY,
    LD_STATUS_ERROR_TIMEOUT,
    LD_STATUS_ERROR_MEMORY,
    LD_STATUS_ERROR_NETWORK,
    LD_STATUS_ERROR_CLOUD_SERVICE,
    LD_STATUS_ERROR_CACHE_FULL,        // Added for cache management
    LD_STATUS_ERROR_OFFLINE_ONLY,      // Added for offline-only mode
    LD_STATUS_ERROR_NO_MATCH,          // Added for no language match found
    LD_STATUS_ERROR_CONFIDENCE_LOW,    // Added for low confidence results
    LD_STATUS_ERROR_MODEL_NOT_LOADED,  // Added for missing language model
    LD_STATUS_ERROR_CACHE_CORRUPTED,   // Added for cache integrity issues
    LD_STATUS_ERROR_FLASH_ACCESS      // Added for flash storage issues
} ld_status_t;

/**
 * @brief Language Detection mode
 */
typedef enum {
    LD_MODE_LOCAL_ONLY = 0,    // Use only local detection (default)
    LD_MODE_HYBRID_LOCAL,      // Prefer local, use cloud for verification only
    LD_MODE_HYBRID_CLOUD,      // Use both with cloud preference
    LD_MODE_CLOUD_ONLY         // Use only cloud detection (not recommended for Follower)
} ld_mode_t;

/**
 * @brief Language Detection confidence level
 */
typedef enum {
    LD_CONFIDENCE_LOW = 0,
    LD_CONFIDENCE_MEDIUM,
    LD_CONFIDENCE_HIGH,
    LD_CONFIDENCE_VERIFIED     // Added for double-verified results
} ld_confidence_t;

/**
 * @brief Local detection method
 */
typedef enum {
    LD_METHOD_NGRAM = 0,      // N-gram based detection
    LD_METHOD_FREQ = 1,       // Character frequency analysis
    LD_METHOD_COMBINED = 2,   // Combined approach (default)
    LD_METHOD_MINIMAL = 3     // Minimal processing for resource constraints
} ld_method_t;

/**
 * @brief Language Detection result
 */
typedef struct {
    char language_code[8];      // ISO 639-1/2 language code
    float confidence;           // Confidence score (0.0-1.0)
    ld_confidence_t level;      // Confidence level
    bool is_cloud_result;       // Whether result came from cloud service
    uint32_t detection_time_ms; // Time taken for detection
    bool from_cache;           // Whether result came from cache
    uint32_t cache_age_ms;     // Age of cached result if applicable
    char secondary_lang[8];    // Secondary language detection
    float secondary_confidence; // Secondary language confidence
} ld_result_t;

/**
 * @brief Cache configuration
 */
typedef struct {
    bool enable_caching;          // Enable result caching
    uint32_t cache_size;          // Maximum cache entries
    uint32_t cache_ttl_ms;        // Cache time-to-live
    float min_cache_confidence;   // Minimum confidence for caching
    bool persist_cache;           // Save cache to flash
    uint32_t cleanup_interval_ms; // Cache cleanup interval
} ld_cache_config_t;

/**
 * @brief Language Detection configuration
 */
typedef struct {
    ld_mode_t mode;                    // Detection mode
    ld_method_t local_method;          // Local detection method
    uint32_t local_min_text_length;    // Minimum text length for local detection
    uint32_t cloud_min_text_length;    // Minimum text length for cloud detection
    uint32_t cloud_timeout_ms;         // Cloud service timeout
    float confidence_threshold;         // Minimum confidence threshold (0.0-1.0)
    ld_cache_config_t cache_config;    // Cache configuration
    char **supported_languages;         // Array of supported language codes
    uint32_t supported_languages_count; // Number of supported languages
    bool prioritize_offline;           // Prioritize offline operation
    bool aggressive_caching;           // Cache more aggressively
    uint32_t offline_model_size_kb;    // Size allocated for offline models
} ld_config_t;

/**
 * @brief Language Detection event types
 */
typedef enum {
    LD_EVENT_DETECTION_COMPLETE = 0,
    LD_EVENT_DETECTION_FAILED,
    LD_EVENT_CLOUD_UNAVAILABLE,
    LD_EVENT_CACHE_HIT,
    LD_EVENT_CACHE_MISS,
    LD_EVENT_CACHE_UPDATED,          // Added for cache monitoring
    LD_EVENT_OFFLINE_FALLBACK,       // Added for offline mode tracking
    LD_EVENT_MODEL_UPDATED,          // Added for model updates
    LD_EVENT_OFFLINE_MODE_ENTERED,   // Added for mode tracking
    LD_EVENT_OFFLINE_MODE_EXITED    // Added for mode tracking
} ld_event_type_t;

/**
 * @brief Language Detection event data
 */
typedef struct {
    ld_event_type_t type;
    ld_result_t result;
    void *user_data;
} ld_event_t;

/**
 * @brief Language Detection event callback
 */
typedef void (*ld_event_callback_t)(ld_event_t *event, void *user_data);

/**
 * @brief Cache statistics
 */
typedef struct {
    uint32_t hits;               // Cache hit count
    uint32_t misses;            // Cache miss count
    uint32_t updates;           // Cache update count
    uint32_t entries;           // Current number of entries
    uint32_t size_bytes;        // Current cache size
    float hit_rate;            // Cache hit rate
    uint32_t avg_lookup_time_us; // Average lookup time
} ld_cache_stats_t;

/**
 * @brief Performance statistics
 */
typedef struct {
    uint32_t local_detections;    // Number of local detections
    uint32_t cloud_detections;    // Number of cloud detections
    uint32_t fallbacks;           // Number of fallbacks to offline
    uint32_t avg_local_time_ms;   // Average local detection time
    uint32_t avg_cloud_time_ms;   // Average cloud detection time
    float local_confidence_avg;   // Average local confidence
    float cloud_confidence_avg;   // Average cloud confidence
} ld_perf_stats_t;

// Function declarations
ld_status_t ld_init(ld_config_t *config);
ld_status_t ld_deinit(void);
ld_status_t ld_register_callback(ld_event_type_t event_type,
                                ld_event_callback_t callback,
                                void *user_data);
ld_status_t ld_unregister_callback(ld_event_type_t event_type,
                                  ld_event_callback_t callback);
ld_status_t ld_detect_language_sync(const char *text,
                                   uint32_t text_length,
                                   ld_result_t *result);
ld_status_t ld_detect_language_async(const char *text,
                                    uint32_t text_length,
                                    void *user_data);
ld_status_t ld_set_mode(ld_mode_t mode);
ld_status_t ld_get_mode(ld_mode_t *mode);
ld_status_t ld_clear_cache(void);
ld_status_t ld_get_supported_languages(char **languages,
                                     uint32_t max_count,
                                     uint32_t *count);
ld_status_t ld_get_cache_stats(ld_cache_stats_t *stats);
ld_status_t ld_get_perf_stats(ld_perf_stats_t *stats);
ld_status_t ld_force_offline_mode(bool enable);
ld_status_t ld_update_local_params(ld_method_t method, float min_confidence);
ld_status_t ld_load_language_model(const char *language_code,
                                  const uint8_t *model_data,
                                  size_t model_size);

#endif /* TOFU_LANGUAGE_DETECTION_H */
