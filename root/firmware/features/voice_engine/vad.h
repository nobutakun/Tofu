/**
 * @file vad.h
 * @brief Voice Activity Detection interface
 * 
 * This file defines the interface for the Voice Activity Detection (VAD)
 * module that provides local processing capabilities for detecting speech
 * segments in audio streams.
 */

#ifndef TOFU_VAD_H
#define TOFU_VAD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief VAD status codes
 */
typedef enum {
    VAD_STATUS_OK = 0,
    VAD_STATUS_ERROR_GENERAL,
    VAD_STATUS_ERROR_NOT_INITIALIZED,
    VAD_STATUS_ERROR_INVALID_PARAM,
    VAD_STATUS_ERROR_NO_SPEECH,
    VAD_STATUS_ERROR_BUFFER_OVERFLOW
} vad_status_t;

/**
 * @brief VAD configuration structure
 */
typedef struct {
    uint32_t sample_rate;         // Audio sample rate in Hz
    uint16_t frame_size_ms;       // Frame size in milliseconds
    uint16_t min_speech_ms;       // Minimum speech duration
    uint16_t max_silence_ms;      // Maximum silence duration
    float energy_threshold;       // Energy threshold for speech detection
    float zcr_threshold;         // Zero crossing rate threshold
    bool adaptive_threshold;     // Enable adaptive thresholding
    bool noise_reduction;        // Enable noise reduction
} vad_config_t;

/**
 * @brief VAD processing result
 */
typedef struct {
    bool is_speech;              // True if speech detected
    float energy;               // Frame energy level
    float confidence;           // Detection confidence [0.0, 1.0]
} vad_result_t;

/**
 * @brief Initialize the VAD module
 * 
 * @param config Pointer to VAD configuration
 * @return vad_status_t Status code
 */
vad_status_t vad_init(const vad_config_t *config);

/**
 * @brief Process an audio frame for voice activity
 * 
 * @param samples Audio samples (16-bit PCM)
 * @param count Number of samples
 * @param result Pointer to store detection result
 * @return vad_status_t Status code
 */
vad_status_t vad_process_frame(const int16_t *samples, size_t count, vad_result_t *result);

/**
 * @brief Reset VAD state
 * 
 * This function resets all internal state of the VAD module,
 * clearing history buffers and detection state.
 * 
 * @return vad_status_t Status code
 */
vad_status_t vad_reset(void);

#endif /* TOFU_VAD_H */
