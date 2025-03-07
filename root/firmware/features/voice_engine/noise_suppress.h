/**
 * @file noise_suppress.h
 * @brief Noise Suppression interface
 * 
 * This file defines the interface for the Noise Suppression module that
 * provides local noise reduction capabilities for improving voice quality
 * and recognition accuracy.
 */

#ifndef TOFU_NOISE_SUPPRESS_H
#define TOFU_NOISE_SUPPRESS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Noise suppression status codes
 */
typedef enum {
    NS_STATUS_OK = 0,
    NS_STATUS_ERROR_GENERAL,
    NS_STATUS_ERROR_NOT_INITIALIZED,
    NS_STATUS_ERROR_INVALID_PARAM,
    NS_STATUS_ERROR_BUFFER_OVERFLOW
} ns_status_t;

/**
 * @brief Noise suppression method
 */
typedef enum {
    NS_METHOD_SPECTRAL = 0,    // Spectral subtraction
    NS_METHOD_WIENER = 1,      // Wiener filtering
    NS_METHOD_MINIMAL = 2      // Minimal processing for low resource usage
} ns_method_t;

/**
 * @brief Noise suppression configuration
 */
typedef struct {
    uint32_t sample_rate;         // Audio sample rate in Hz
    uint16_t frame_size_ms;       // Frame size in milliseconds
    ns_method_t method;           // Noise reduction method
    float aggressiveness;         // Noise reduction aggressiveness [0.0, 1.0]
    bool adapt_to_noise;          // Adapt to changing noise conditions
    uint16_t noise_learn_ms;      // Initial noise learning period
    float min_signal_db;          // Minimum signal level to process
    bool preserve_voice;          // Prioritize voice preservation over noise reduction
} ns_config_t;

/**
 * @brief Processing statistics
 */
typedef struct {
    float current_snr;           // Current signal-to-noise ratio (dB)
    float noise_floor;           // Estimated noise floor
    float signal_level;          // Current signal level
    bool speech_detected;        // Speech activity indicator
    uint32_t processed_frames;   // Number of frames processed
} ns_stats_t;

/**
 * @brief Initialize the noise suppression module
 * 
 * @param config Pointer to configuration structure
 * @return ns_status_t Status code
 */
ns_status_t ns_init(const ns_config_t *config);

/**
 * @brief Process an audio frame for noise reduction
 * 
 * @param input Input audio samples (16-bit PCM)
 * @param output Buffer for processed output samples
 * @param sample_count Number of samples to process
 * @return ns_status_t Status code
 */
ns_status_t ns_process_frame(const int16_t *input, int16_t *output, size_t sample_count);

/**
 * @brief Get current processing statistics
 * 
 * @param stats Pointer to statistics structure to fill
 * @return ns_status_t Status code
 */
ns_status_t ns_get_stats(ns_stats_t *stats);

/**
 * @brief Reset noise suppression state
 * 
 * This function resets all internal state, including noise floor estimates
 * and processing history.
 * 
 * @return ns_status_t Status code
 */
ns_status_t ns_reset(void);

/**
 * @brief Update noise suppression parameters
 * 
 * @param aggressiveness New aggressiveness value [0.0, 1.0]
 * @param preserve_voice Prioritize voice preservation
 * @return ns_status_t Status code
 */
ns_status_t ns_update_params(float aggressiveness, bool preserve_voice);

/**
 * @brief Force noise floor recalibration
 * 
 * This function triggers a new noise floor estimation period.
 * 
 * @return ns_status_t Status code
 */
ns_status_t ns_recalibrate(void);

#endif /* TOFU_NOISE_SUPPRESS_H */
