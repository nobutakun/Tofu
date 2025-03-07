/**
 * @file keyword_detect.h
 * @brief Keyword Detection interface
 * 
 * This file defines the interface for the Keyword Detection module that
 * provides local wake-word detection and basic command recognition
 * capabilities.
 */

#ifndef TOFU_KEYWORD_DETECT_H
#define TOFU_KEYWORD_DETECT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Maximum number of keywords that can be registered
 */
#define MAX_KEYWORDS 10

/**
 * @brief Maximum length of a keyword phrase in samples
 */
#define MAX_KEYWORD_LENGTH_MS 2000

/**
 * @brief Keyword detection status codes
 */
typedef enum {
    KWD_STATUS_OK = 0,
    KWD_STATUS_ERROR_GENERAL,
    KWD_STATUS_ERROR_NOT_INITIALIZED,
    KWD_STATUS_ERROR_INVALID_PARAM,
    KWD_STATUS_ERROR_MAX_KEYWORDS,
    KWD_STATUS_ERROR_NO_MATCH,
    KWD_STATUS_ERROR_BUFFER_FULL,
    KWD_STATUS_ERROR_MEMORY        // Added for memory allocation errors
} kwd_status_t;

/**
 * @brief Keyword matching algorithm type
 */
typedef enum {
    KWD_ALGO_DTW = 0,          // Dynamic Time Warping
    KWD_ALGO_MFCC = 1,         // MFCC with pattern matching
    KWD_ALGO_PHONEME = 2,      // Phoneme-based matching
    KWD_ALGO_MINIMAL = 3       // Minimal memory usage mode
} kwd_algorithm_t;

/**
 * @brief Keyword configuration structure
 */
typedef struct {
    uint32_t sample_rate;           // Audio sample rate in Hz
    uint16_t frame_size_ms;         // Processing frame size
    float detection_threshold;      // Detection confidence threshold
    kwd_algorithm_t algorithm;      // Matching algorithm to use
    bool use_prefilter;            // Enable noise pre-filtering
    bool adaptive_threshold;        // Enable adaptive thresholding
    uint16_t max_phrase_ms;        // Maximum phrase duration
    bool cache_templates;          // Enable template caching
    uint32_t cache_size_kb;        // Template cache size in KB
} kwd_config_t;

/**
 * @brief Keyword detection result
 */
typedef struct {
    uint8_t keyword_index;         // Index of matched keyword
    float confidence;              // Match confidence [0.0, 1.0]
    uint32_t start_sample;         // Start sample of detection
    uint32_t end_sample;           // End sample of detection
    bool is_verified;             // Secondary verification passed
    bool from_cache;              // Result from cached template
    uint32_t process_time_us;     // Processing time in microseconds
} kwd_result_t;

/**
 * @brief Performance statistics
 */
typedef struct {
    uint32_t cache_hits;          // Number of template cache hits
    uint32_t cache_misses;        // Number of template cache misses
    uint32_t avg_process_time_us; // Average processing time
    uint32_t max_process_time_us; // Maximum processing time
    float cache_hit_rate;         // Cache hit rate [0.0, 1.0]
    uint32_t memory_used_kb;      // Memory usage in KB
    uint32_t templates_loaded;     // Number of loaded templates
} kwd_stats_t;

// Function declarations
kwd_status_t kwd_init(const kwd_config_t *config);
kwd_status_t kwd_register_keyword(const uint8_t *keyword_data,
                                 size_t size,
                                 uint8_t *keyword_id);
kwd_status_t kwd_remove_keyword(uint8_t keyword_id);
kwd_status_t kwd_process_frame(const int16_t *samples,
                              size_t count,
                              kwd_result_t *result);
kwd_status_t kwd_reset(void);
kwd_status_t kwd_get_keyword_count(uint8_t *count);
kwd_status_t kwd_set_params(float threshold, bool use_prefilter);
kwd_status_t kwd_get_stats(kwd_stats_t *stats);
kwd_status_t kwd_optimize_memory(bool aggressive);
kwd_status_t kwd_preload_templates(void);

#endif /* TOFU_KEYWORD_DETECT_H */
