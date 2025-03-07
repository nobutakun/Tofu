/**
 * @file vad.c
 * @brief Voice Activity Detection implementation for offline processing
 * 
 * This file implements voice activity detection using local processing
 * to identify speech segments without cloud dependency.
 */

#include "vad.h"
#include "firmware_config.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Configuration constants
#define VAD_FRAME_SIZE_MS        30
#define VAD_MIN_SPEECH_MS        100
#define VAD_MAX_SILENCE_MS       500
#define VAD_ENERGY_THRESHOLD     0.1f
#define VAD_ZCR_THRESHOLD        0.2f
#define VAD_HISTORY_SIZE         3

// Internal state
typedef struct {
    bool is_active;
    uint32_t active_frames;
    uint32_t silence_frames;
    float energy_history[VAD_HISTORY_SIZE];
    float zcr_history[VAD_HISTORY_SIZE];
    uint8_t history_index;
} vad_state_t;

static vad_state_t vad_state = {0};

/**
 * @brief Initialize VAD module
 * 
 * @param config Pointer to VAD configuration
 * @return vad_status_t Status code
 */
vad_status_t vad_init(const vad_config_t *config)
{
    if (!config) {
        return VAD_STATUS_ERROR_INVALID_PARAM;
    }

    memset(&vad_state, 0, sizeof(vad_state_t));
    return VAD_STATUS_OK;
}

/**
 * @brief Calculate signal energy
 * 
 * @param samples Audio samples
 * @param count Number of samples
 * @return float Signal energy value
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
 * @brief Calculate zero crossing rate
 * 
 * @param samples Audio samples
 * @param count Number of samples
 * @return float Zero crossing rate
 */
static float calculate_zcr(const int16_t *samples, size_t count)
{
    int crossings = 0;
    for (size_t i = 1; i < count; i++) {
        if ((samples[i] >= 0 && samples[i-1] < 0) ||
            (samples[i] < 0 && samples[i-1] >= 0)) {
            crossings++;
        }
    }
    return (float)crossings / count;
}

/**
 * @brief Update VAD history buffers
 * 
 * @param energy Current energy value
 * @param zcr Current zero crossing rate
 */
static void update_history(float energy, float zcr)
{
    vad_state.energy_history[vad_state.history_index] = energy;
    vad_state.zcr_history[vad_state.history_index] = zcr;
    vad_state.history_index = (vad_state.history_index + 1) % VAD_HISTORY_SIZE;
}

/**
 * @brief Get average from history buffer
 * 
 * @param buffer History buffer
 * @return float Average value
 */
static float get_history_average(const float *buffer)
{
    float sum = 0.0f;
    for (int i = 0; i < VAD_HISTORY_SIZE; i++) {
        sum += buffer[i];
    }
    return sum / VAD_HISTORY_SIZE;
}

/**
 * @brief Process audio frame to detect voice activity
 * 
 * @param samples Audio samples
 * @param count Number of samples
 * @param result Pointer to store detection result
 * @return vad_status_t Status code
 */
vad_status_t vad_process_frame(const int16_t *samples, size_t count, vad_result_t *result)
{
    if (!samples || !result || count == 0) {
        return VAD_STATUS_ERROR_INVALID_PARAM;
    }

    // Calculate frame metrics
    float energy = calculate_energy(samples, count);
    float zcr = calculate_zcr(samples, count);
    
    // Update history
    update_history(energy, zcr);
    
    // Get smoothed values
    float avg_energy = get_history_average(vad_state.energy_history);
    float avg_zcr = get_history_average(vad_state.zcr_history);
    
    // Speech detection logic
    bool speech_detected = (avg_energy > VAD_ENERGY_THRESHOLD) &&
                         (avg_zcr > VAD_ZCR_THRESHOLD);
    
    // State machine update
    if (speech_detected) {
        if (!vad_state.is_active) {
            vad_state.is_active = true;
        }
        vad_state.active_frames++;
        vad_state.silence_frames = 0;
    } else {
        if (vad_state.is_active) {
            vad_state.silence_frames++;
            if (vad_state.silence_frames * VAD_FRAME_SIZE_MS >= VAD_MAX_SILENCE_MS) {
                vad_state.is_active = false;
                vad_state.active_frames = 0;
            }
        }
    }
    
    // Set result
    result->is_speech = vad_state.is_active;
    result->energy = avg_energy;
    result->confidence = avg_energy / (VAD_ENERGY_THRESHOLD * 2.0f);
    if (result->confidence > 1.0f) {
        result->confidence = 1.0f;
    }
    
    return VAD_STATUS_OK;
}

/**
 * @brief Reset VAD state
 * 
 * @return vad_status_t Status code
 */
vad_status_t vad_reset(void)
{
    memset(&vad_state, 0, sizeof(vad_state_t));
    return VAD_STATUS_OK;
}
