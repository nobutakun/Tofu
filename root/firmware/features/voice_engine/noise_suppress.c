/**
 * @file noise_suppress.c
 * @brief Noise Suppression implementation for offline voice processing
 * 
 * This file implements basic noise suppression techniques suitable for
 * embedded systems, focusing on local processing without cloud dependency.
 */

#include "noise_suppress.h"
#include "firmware_config.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Configuration constants
#define NS_FRAME_SIZE          512
#define NS_HISTORY_SIZE        5
#define NS_MIN_ENERGY         0.001f
#define NS_ALPHA             0.95f    // Smoothing factor
#define NS_BETA              1.2f     // Oversubtraction factor
#define NS_MIN_SNR          -5.0f     // Minimum SNR in dB
#define NS_MAX_GAIN_DB      30.0f     // Maximum gain reduction
#define NS_MIN_SPEECH_DB    -30.0f    // Minimum speech level

// Internal state structure
typedef struct {
    bool initialized;
    ns_config_t config;
    float noise_floor;
    float energy_history[NS_HISTORY_SIZE];
    uint8_t history_index;
    uint32_t frame_count;
    float prev_gain;
    float *temp_buffer;
    size_t buffer_size;
    ns_stats_t stats;
    bool in_speech;
    uint32_t noise_learn_frames;
} ns_state_t;

static ns_state_t ns_state = {0};

/**
 * @brief Calculate frame energy
 */
static float calculate_energy(const int16_t *samples, size_t count)
{
    float energy = 0.0f;
    for (size_t i = 0; i < count; i++) {
        float sample = samples[i] / 32768.0f;  // Normalize to [-1,1]
        energy += sample * sample;
    }
    return energy / count;
}

/**
 * @brief Convert linear to dB
 */
static float linear_to_db(float value)
{
    return 20.0f * log10f(value + 1e-10f);
}

/**
 * @brief Convert dB to linear
 */
static float db_to_linear(float db)
{
    return powf(10.0f, db / 20.0f);
}

/**
 * @brief Update noise floor estimate
 */
static void update_noise_floor(float frame_energy)
{
    // Update history buffer
    ns_state.energy_history[ns_state.history_index] = frame_energy;
    ns_state.history_index = (ns_state.history_index + 1) % NS_HISTORY_SIZE;
    
    // Calculate minimum energy from history
    float min_energy = ns_state.energy_history[0];
    for (int i = 1; i < NS_HISTORY_SIZE; i++) {
        if (ns_state.energy_history[i] < min_energy) {
            min_energy = ns_state.energy_history[i];
        }
    }
    
    // During initial learning period, aggressively track noise
    if (ns_state.frame_count < ns_state.noise_learn_frames) {
        ns_state.noise_floor = min_energy;
        return;
    }
    
    // Adaptive noise floor tracking
    if (ns_state.config.adapt_to_noise) {
        float alpha = ns_state.in_speech ? 0.99f : NS_ALPHA;
        ns_state.noise_floor = alpha * ns_state.noise_floor +
                             (1.0f - alpha) * min_energy;
    }
    
    if (ns_state.noise_floor < NS_MIN_ENERGY) {
        ns_state.noise_floor = NS_MIN_ENERGY;
    }
}

/**
 * @brief Initialize noise suppression
 */
ns_status_t ns_init(const ns_config_t *config)
{
    if (!config) {
        return NS_STATUS_ERROR_INVALID_PARAM;
    }

    memset(&ns_state, 0, sizeof(ns_state_t));
    memcpy(&ns_state.config, config, sizeof(ns_config_t));
    
    // Calculate frames for noise learning period
    ns_state.noise_learn_frames = (config->noise_learn_ms * config->sample_rate) /
                                 (1000 * NS_FRAME_SIZE);
    
    // Allocate temporary buffer
    ns_state.buffer_size = NS_FRAME_SIZE * sizeof(float);
    ns_state.temp_buffer = malloc(ns_state.buffer_size);
    if (!ns_state.temp_buffer) {
        return NS_STATUS_ERROR_GENERAL;
    }
    
    ns_state.noise_floor = NS_MIN_ENERGY;
    ns_state.prev_gain = 1.0f;
    ns_state.initialized = true;
    
    return NS_STATUS_OK;
}

/**
 * @brief Process audio frame for noise reduction
 */
ns_status_t ns_process_frame(const int16_t *input, int16_t *output, size_t sample_count)
{
    if (!ns_state.initialized || !input || !output || sample_count == 0) {
        return NS_STATUS_ERROR_INVALID_PARAM;
    }

    if (sample_count > NS_FRAME_SIZE) {
        return NS_STATUS_ERROR_BUFFER_OVERFLOW;
    }

    // Calculate frame energy
    float frame_energy = calculate_energy(input, sample_count);
    float frame_db = linear_to_db(sqrtf(frame_energy));
    
    // Update noise floor estimate
    update_noise_floor(frame_energy);
    
    // Calculate SNR
    float noise_db = linear_to_db(sqrtf(ns_state.noise_floor));
    float snr = frame_db - noise_db;
    
    // Detect speech activity
    ns_state.in_speech = (snr > ns_state.config.min_signal_db);
    
    // Calculate gain based on selected method
    float gain_db = 0.0f;
    switch (ns_state.config.method) {
        case NS_METHOD_SPECTRAL:
            // Simple spectral subtraction
            gain_db = -NS_BETA * noise_db * ns_state.config.aggressiveness;
            break;
            
        case NS_METHOD_WIENER:
            // Simplified Wiener filter
            if (snr > 0) {
                gain_db = -10.0f * log10f(1.0f + 1.0f/powf(10.0f, snr/10.0f));
            } else {
                gain_db = -NS_MAX_GAIN_DB;
            }
            gain_db *= ns_state.config.aggressiveness;
            break;
            
        case NS_METHOD_MINIMAL:
            // Very simple threshold-based reduction
            if (snr < 0) {
                gain_db = snr * ns_state.config.aggressiveness;
            }
            break;
    }
    
    // Limit maximum attenuation
    if (gain_db < -NS_MAX_GAIN_DB) {
        gain_db = -NS_MAX_GAIN_DB;
    }
    
    // Smooth gain changes
    float gain = db_to_linear(gain_db);
    gain = 0.7f * ns_state.prev_gain + 0.3f * gain;
    ns_state.prev_gain = gain;
    
    // If preserving voice, reduce gain reduction during speech
    if (ns_state.config.preserve_voice && ns_state.in_speech) {
        gain = sqrtf(gain);  // Less aggressive reduction
    }
    
    // Apply gain
    for (size_t i = 0; i < sample_count; i++) {
        float sample = input[i] * gain;
        // Clipping prevention
        if (sample > 32767.0f) sample = 32767.0f;
        if (sample < -32768.0f) sample = -32768.0f;
        output[i] = (int16_t)sample;
    }
    
    // Update statistics
    ns_state.stats.current_snr = snr;
    ns_state.stats.noise_floor = noise_db;
    ns_state.stats.signal_level = frame_db;
    ns_state.stats.speech_detected = ns_state.in_speech;
    ns_state.stats.processed_frames = ++ns_state.frame_count;
    
    return NS_STATUS_OK;
}

/**
 * @brief Get current processing statistics
 */
ns_status_t ns_get_stats(ns_stats_t *stats)
{
    if (!ns_state.initialized || !stats) {
        return NS_STATUS_ERROR_INVALID_PARAM;
    }
    
    memcpy(stats, &ns_state.stats, sizeof(ns_stats_t));
    return NS_STATUS_OK;
}

/**
 * @brief Reset noise suppression state
 */
ns_status_t ns_reset(void)
{
    if (!ns_state.initialized) {
        return NS_STATUS_ERROR_NOT_INITIALIZED;
    }
    
    // Reset noise floor and history
    ns_state.noise_floor = NS_MIN_ENERGY;
    memset(ns_state.energy_history, 0, sizeof(ns_state.energy_history));
    ns_state.history_index = 0;
    
    // Reset statistics
    memset(&ns_state.stats, 0, sizeof(ns_stats_t));
    ns_state.frame_count = 0;
    ns_state.in_speech = false;
    ns_state.prev_gain = 1.0f;
    
    return NS_STATUS_OK;
}

/**
 * @brief Update noise suppression parameters
 */
ns_status_t ns_update_params(float aggressiveness, bool preserve_voice)
{
    if (!ns_state.initialized) {
        return NS_STATUS_ERROR_NOT_INITIALIZED;
    }
    
    if (aggressiveness < 0.0f || aggressiveness > 1.0f) {
        return NS_STATUS_ERROR_INVALID_PARAM;
    }
    
    ns_state.config.aggressiveness = aggressiveness;
    ns_state.config.preserve_voice = preserve_voice;
    
    return NS_STATUS_OK;
}

/**
 * @brief Force noise floor recalibration
 */
ns_status_t ns_recalibrate(void)
{
    if (!ns_state.initialized) {
        return NS_STATUS_ERROR_NOT_INITIALIZED;
    }
    
    // Reset noise floor estimate and frame counter
    ns_state.noise_floor = NS_MIN_ENERGY;
    ns_state.frame_count = 0;
    memset(ns_state.energy_history, 0, sizeof(ns_state.energy_history));
    ns_state.history_index = 0;
    
    return NS_STATUS_OK;
}
