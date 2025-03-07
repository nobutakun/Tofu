/**
 * @file keyword_detect.c
 * @brief Keyword Detection implementation with template caching
 * 
 * This implementation uses Dynamic Time Warping (DTW) and template caching
 * for efficient keyword matching, optimized for offline operation in
 * embedded systems.
 */

#include "keyword_detect.h"
#include "../../firmware_config.h"
#include "../../system_manager.h"
#include "../../hal.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

// Configuration constants
#define MAX_FRAME_SIZE          512
#define DTW_WINDOW_SIZE        0.1f
#define MIN_KEYWORD_FRAMES     10
#define FEATURE_VECTOR_SIZE    13
#define PREEMPHASIS_ALPHA     0.97f
#define VERIFICATION_THRESHOLD 0.85f
#define TEMPLATE_CACHE_SIZE    5
#define M_PI                  3.14159265358979323846f

// DTW cost matrix dimensions
#define DTW_MAX_TEMPLATE_FRAMES 200
#define DTW_MAX_INPUT_FRAMES    200

// Memory optimization - use 16-bit fixed point for feature storage
#define FEATURE_SCALE          32767.0f
#define FIXED_TO_FLOAT(x)      ((float)(x) / FEATURE_SCALE)
#define FLOAT_TO_FIXED(x)      ((int16_t)((x) * FEATURE_SCALE))

// Internal structures
typedef struct {
    int16_t features[FEATURE_VECTOR_SIZE];  // Fixed-point features
} feature_vector_t;

typedef struct {
    uint8_t *data;
    size_t size;
    feature_vector_t *template_features;
    uint32_t frame_count;
    bool is_active;
    uint32_t hit_count;          // Cache hit counter
    uint64_t last_access;        // Last access timestamp
} keyword_template_t;

// Module state structure
typedef struct {
    bool initialized;
    kwd_config_t config;
    keyword_template_t keywords[MAX_KEYWORDS];
    uint8_t keyword_count;
    uint16_t *dtw_cost_matrix;   // Use 16-bit fixed point
    float *input_buffer;
    size_t buffer_size;
    size_t buffer_pos;
    
    // Template cache
    struct {
        uint8_t template_indices[TEMPLATE_CACHE_SIZE];
        uint32_t hit_count;
        uint32_t miss_count;
        kwd_stats_t stats;
    } cache;
} kwd_state_t;

// Module instance
static kwd_state_t kwd_state = {0};

// Forward declarations
static void extract_features(const float *frame, size_t count, feature_vector_t *features);
static float calculate_dtw(const feature_vector_t *template_seq, uint32_t template_len,
                          const feature_vector_t *input_seq, uint32_t input_len);
static bool is_template_cached(uint8_t template_index);
static void update_template_cache(uint8_t template_index);
static void load_template_to_cache(uint8_t template_index);

/**
 * @brief Extract features from audio frame
 */
static void extract_features(const float *frame, size_t count, feature_vector_t *features) {
    if (count < MAX_FRAME_SIZE) {
        return;
    }

    // Pre-emphasis
    float emphasized[MAX_FRAME_SIZE];
    emphasized[0] = frame[0];
    for (size_t i = 1; i < count; i++) {
        emphasized[i] = frame[i] - PREEMPHASIS_ALPHA * frame[i-1];
    }

    // Window the frame (Hamming window)
    float windowed[MAX_FRAME_SIZE];
    for (size_t i = 0; i < count; i++) {
        float a = 2.0f * M_PI * i / (count - 1);
        windowed[i] = emphasized[i] * (0.54f - 0.46f * cosf(a));
    }

    // Calculate log energy as first feature
    float energy = 0.0f;
    for (size_t i = 0; i < count; i++) {
        energy += windowed[i] * windowed[i];
    }
    features->features[0] = FLOAT_TO_FIXED(logf(energy + 1e-10f));

    // Simplified feature extraction for remaining features
    for (int i = 1; i < FEATURE_VECTOR_SIZE; i++) {
        float sum = 0.0f;
        for (size_t j = 0; j < count; j += FEATURE_VECTOR_SIZE) {
            sum += windowed[j + (i % count)];
        }
        features->features[i] = FLOAT_TO_FIXED(sum / count);
    }
}

/**
 * @brief Check if template is cached
 */
static bool is_template_cached(uint8_t template_index) {
    for (int i = 0; i < TEMPLATE_CACHE_SIZE; i++) {
        if (kwd_state.cache.template_indices[i] == template_index) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Update template cache based on usage
 */
static void update_template_cache(uint8_t template_index) {
    if (is_template_cached(template_index)) {
        kwd_state.cache.hit_count++;
        kwd_state.cache.stats.cache_hits++;
        kwd_state.keywords[template_index].hit_count++;
        kwd_state.keywords[template_index].last_access = sys_get_time_ms();
        return;
    }

    kwd_state.cache.miss_count++;
    kwd_state.cache.stats.cache_misses++;
    
    uint8_t lru_slot = 0;
    uint64_t oldest_access = UINT64_MAX;
    
    for (int i = 0; i < TEMPLATE_CACHE_SIZE; i++) {
        uint8_t cached_index = kwd_state.cache.template_indices[i];
        if (cached_index == 0xFF) {
            lru_slot = i;
            break;
        }
        if (kwd_state.keywords[cached_index].last_access < oldest_access) {
            oldest_access = kwd_state.keywords[cached_index].last_access;
            lru_slot = i;
        }
    }

    load_template_to_cache(template_index);
    kwd_state.cache.template_indices[lru_slot] = template_index;
    kwd_state.keywords[template_index].last_access = sys_get_time_ms();
}

/**
 * @brief Load template data into cache
 */
static void load_template_to_cache(uint8_t template_index) {
    if (!kwd_state.keywords[template_index].template_features) {
        size_t num_frames = kwd_state.keywords[template_index].size / sizeof(float);
        kwd_state.keywords[template_index].template_features = 
            malloc(num_frames * sizeof(feature_vector_t));
        
        if (!kwd_state.keywords[template_index].template_features) {
            return;
        }
        
        const float *template_data = (const float *)kwd_state.keywords[template_index].data;
        for (size_t i = 0; i < num_frames; i++) {
            extract_features(&template_data[i * MAX_FRAME_SIZE], MAX_FRAME_SIZE,
                           &kwd_state.keywords[template_index].template_features[i]);
        }
        
        kwd_state.keywords[template_index].frame_count = num_frames;
    }

    kwd_state.cache.stats.memory_used_kb = 
        (sizeof(feature_vector_t) * kwd_state.keywords[template_index].frame_count +
         kwd_state.keywords[template_index].size) / 1024;
}

/**
 * @brief Calculate DTW distance between template and input
 */
static float calculate_dtw(const feature_vector_t *template_seq, uint32_t template_len,
                          const feature_vector_t *input_seq, uint32_t input_len) {
    memset(kwd_state.dtw_cost_matrix, 0xFF, 
           DTW_MAX_TEMPLATE_FRAMES * DTW_MAX_INPUT_FRAMES * sizeof(uint16_t));
    kwd_state.dtw_cost_matrix[0] = 0;

    float window = DTW_WINDOW_SIZE * template_len;
    
    for (uint32_t i = 1; i < template_len; i++) {
        uint32_t start = (uint32_t)fmaxf(1, i - window);
        uint32_t end = (uint32_t)fminf(input_len, i + window);
        
        for (uint32_t j = start; j < end; j++) {
            uint32_t dist = 0;
            for (int k = 0; k < FEATURE_VECTOR_SIZE; k++) {
                int32_t diff = template_seq[i].features[k] - input_seq[j].features[k];
                dist += (diff * diff) >> 8;
            }
            
            uint16_t min_cost = UINT16_MAX;
            if (i > 0 && j > 0) {
                uint16_t d = kwd_state.dtw_cost_matrix[(i-1) * input_len + (j-1)];
                uint16_t h = kwd_state.dtw_cost_matrix[i * input_len + (j-1)];
                uint16_t v = kwd_state.dtw_cost_matrix[(i-1) * input_len + j];
                
                min_cost = d;
                if (h < min_cost) min_cost = h;
                if (v < min_cost) min_cost = v;
            }
            
            uint32_t total_cost = dist + min_cost;
            if (total_cost > UINT16_MAX) total_cost = UINT16_MAX;
            
            kwd_state.dtw_cost_matrix[i * input_len + j] = (uint16_t)total_cost;
        }
    }

    float final_cost = (float)kwd_state.dtw_cost_matrix[template_len * input_len - 1];
    return final_cost / (template_len + input_len);
}

/**
 * @brief Initialize the Keyword Detection module
 */
kwd_status_t kwd_init(const kwd_config_t *config) {
    if (!config) {
        return KWD_STATUS_ERROR_INVALID_PARAM;
    }

    if (kwd_state.initialized) {
        kwd_reset();
    }

    // Initialize state
    memset(&kwd_state, 0, sizeof(kwd_state_t));
    memcpy(&kwd_state.config, config, sizeof(kwd_config_t));

    // Allocate DTW matrix with 16-bit storage
    size_t matrix_size = DTW_MAX_TEMPLATE_FRAMES * DTW_MAX_INPUT_FRAMES;
    kwd_state.dtw_cost_matrix = malloc(matrix_size * sizeof(uint16_t));
    if (!kwd_state.dtw_cost_matrix) {
        return KWD_STATUS_ERROR_MEMORY;
    }

    // Allocate input buffer
    size_t buffer_samples = (config->max_phrase_ms * config->sample_rate) / 1000;
    kwd_state.input_buffer = malloc(buffer_samples * sizeof(float));
    if (!kwd_state.input_buffer) {
        free(kwd_state.dtw_cost_matrix);
        return KWD_STATUS_ERROR_MEMORY;
    }
    kwd_state.buffer_size = buffer_samples;

    // Initialize cache
    for (int i = 0; i < TEMPLATE_CACHE_SIZE; i++) {
        kwd_state.cache.template_indices[i] = 0xFF; // Invalid index
    }

    kwd_state.initialized = true;
    return KWD_STATUS_OK;
}

/**
 * @brief Register a new keyword for detection
 */
kwd_status_t kwd_register_keyword(const uint8_t *keyword_data, size_t size, uint8_t *keyword_id) {
    if (!kwd_state.initialized || !keyword_data || !keyword_id || size == 0) {
        return KWD_STATUS_ERROR_INVALID_PARAM;
    }

    if (kwd_state.keyword_count >= MAX_KEYWORDS) {
        return KWD_STATUS_ERROR_MAX_KEYWORDS;
    }

    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_KEYWORDS; i++) {
        if (!kwd_state.keywords[i].is_active) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        return KWD_STATUS_ERROR_MAX_KEYWORDS;
    }

    // Allocate template storage
    kwd_state.keywords[slot].data = malloc(size);
    if (!kwd_state.keywords[slot].data) {
        return KWD_STATUS_ERROR_MEMORY;
    }

    // Copy template data
    memcpy(kwd_state.keywords[slot].data, keyword_data, size);
    kwd_state.keywords[slot].size = size;
    kwd_state.keywords[slot].is_active = true;
    kwd_state.keyword_count++;
    *keyword_id = (uint8_t)slot;

    // Pre-compute features if caching enabled
    if (kwd_state.config.cache_templates) {
        load_template_to_cache(slot);
    }

    return KWD_STATUS_OK;
}

/**
 * @brief Process audio frame for keyword detection
 */
kwd_status_t kwd_process_frame(const int16_t *samples, size_t count, kwd_result_t *result) {
    if (!kwd_state.initialized || !samples || !result || count == 0) {
        return KWD_STATUS_ERROR_INVALID_PARAM;
    }

    uint64_t start_time = sys_get_time_us();

    // Convert samples to float
    float frame[MAX_FRAME_SIZE];
    size_t frame_count = count > MAX_FRAME_SIZE ? MAX_FRAME_SIZE : count;
    for (size_t i = 0; i < frame_count; i++) {
        frame[i] = samples[i] / 32768.0f;
    }

    // Extract features
    feature_vector_t features;
    extract_features(frame, frame_count, &features);

    // Check against each keyword template
    float best_confidence = 0.0f;
    int best_match = -1;
    bool used_cache = false;

    for (int i = 0; i < MAX_KEYWORDS; i++) {
        if (!kwd_state.keywords[i].is_active) {
            continue;
        }

        // Update cache status
        if (kwd_state.config.cache_templates) {
            update_template_cache(i);
            used_cache = is_template_cached(i);
        }

        // Calculate DTW distance
        float dist = calculate_dtw(kwd_state.keywords[i].template_features,
                                 kwd_state.keywords[i].frame_count,
                                 &features, 1);

        // Convert distance to confidence score
        float confidence = 1.0f / (1.0f + dist);
        if (confidence > best_confidence) {
            best_confidence = confidence;
            best_match = i;
        }
    }

    // Update statistics
    uint32_t process_time = (uint32_t)(sys_get_time_us() - start_time);
    if (process_time > kwd_state.cache.stats.max_process_time_us) {
        kwd_state.cache.stats.max_process_time_us = process_time;
    }
    
    // Update average processing time
    uint32_t total_frames = kwd_state.cache.stats.cache_hits + kwd_state.cache.stats.cache_misses;
    kwd_state.cache.stats.avg_process_time_us = 
        (kwd_state.cache.stats.avg_process_time_us * total_frames + process_time) / (total_frames + 1);

    // Fill result
    if (best_match >= 0 && best_confidence >= kwd_state.config.detection_threshold) {
        result->keyword_index = (uint8_t)best_match;
        result->confidence = best_confidence;
        result->start_sample = 0;
        result->end_sample = count;
        result->is_verified = best_confidence >= VERIFICATION_THRESHOLD;
        result->from_cache = used_cache;
        result->process_time_us = process_time;
        return KWD_STATUS_OK;
    }

    return KWD_STATUS_ERROR_NO_MATCH;
}

/**
 * @brief Get performance statistics
 */
kwd_status_t kwd_get_stats(kwd_stats_t *stats) {
    if (!kwd_state.initialized || !stats) {
        return KWD_STATUS_ERROR_INVALID_PARAM;
    }

    memcpy(stats, &kwd_state.cache.stats, sizeof(kwd_stats_t));
    return KWD_STATUS_OK;
}

/**
 * @brief Reset the Keyword Detection module
 */
kwd_status_t kwd_reset(void) {
    if (!kwd_state.initialized) {
        return KWD_STATUS_ERROR_NOT_INITIALIZED;
    }

    // Free allocated resources
    for (int i = 0; i < MAX_KEYWORDS; i++) {
        if (kwd_state.keywords[i].data) {
            free(kwd_state.keywords[i].data);
        }
        if (kwd_state.keywords[i].template_features) {
            free(kwd_state.keywords[i].template_features);
        }
    }

    free(kwd_state.dtw_cost_matrix);
    free(kwd_state.input_buffer);

    // Clear state
    memset(&kwd_state, 0, sizeof(kwd_state_t));
    return KWD_STATUS_OK;
}

/**
 * @brief Get number of registered keywords
 */
kwd_status_t kwd_get_keyword_count(uint8_t *count) {
    if (!kwd_state.initialized || !count) {
        return KWD_STATUS_ERROR_INVALID_PARAM;
    }

    *count = kwd_state.keyword_count;
    return KWD_STATUS_OK;
}

/**
 * @brief Set detection parameters
 */
kwd_status_t kwd_set_params(float threshold, bool use_prefilter) {
    if (!kwd_state.initialized) {
        return KWD_STATUS_ERROR_NOT_INITIALIZED;
    }

    if (threshold < 0.0f || threshold > 1.0f) {
        return KWD_STATUS_ERROR_INVALID_PARAM;
    }

    kwd_state.config.detection_threshold = threshold;
    kwd_state.config.use_prefilter = use_prefilter;

    return KWD_STATUS_OK;
}

/**
 * @brief Remove a registered keyword
 */
kwd_status_t kwd_remove_keyword(uint8_t keyword_id) {
    if (!kwd_state.initialized || keyword_id >= MAX_KEYWORDS) {
        return KWD_STATUS_ERROR_INVALID_PARAM;
    }

    if (!kwd_state.keywords[keyword_id].is_active) {
        return KWD_STATUS_ERROR_INVALID_PARAM;
    }

    // Free allocated resources
    free(kwd_state.keywords[keyword_id].data);
    free(kwd_state.keywords[keyword_id].template_features);
    
    // Clear state
    memset(&kwd_state.keywords[keyword_id], 0, sizeof(keyword_template_t));
    kwd_state.keyword_count--;

    // Remove from cache if present
    for (int i = 0; i < TEMPLATE_CACHE_SIZE; i++) {
        if (kwd_state.cache.template_indices[i] == keyword_id) {
            kwd_state.cache.template_indices[i] = 0xFF;
            break;
        }
    }

    return KWD_STATUS_OK;
}
