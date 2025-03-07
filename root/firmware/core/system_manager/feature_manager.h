/**
 * @file feature_manager.h
 * @brief Feature Manager for To-fu device
 * 
 * This file defines the Feature Manager interface for the To-fu device.
 * The Feature Manager is responsible for initializing, coordinating, and
 * managing all feature modules, including voice processing, expression
 * handling, and interaction capabilities.
 */

#ifndef TOFU_FEATURE_MANAGER_H
#define TOFU_FEATURE_MANAGER_H

#include "firmware_config.h"
#include "hal.h"
#include "system_manager.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Feature Manager status codes
 */
typedef enum {
    FEATURE_STATUS_OK = 0,
    FEATURE_STATUS_ERROR_GENERAL,
    FEATURE_STATUS_ERROR_NOT_INITIALIZED,
    FEATURE_STATUS_ERROR_ALREADY_INITIALIZED,
    FEATURE_STATUS_ERROR_INVALID_PARAM,
    FEATURE_STATUS_ERROR_NOT_SUPPORTED,
    FEATURE_STATUS_ERROR_RESOURCE_BUSY,
    FEATURE_STATUS_ERROR_TIMEOUT,
    FEATURE_STATUS_ERROR_MEMORY
} feature_status_t;

/**
 * @brief Feature types
 */
typedef enum {
    FEATURE_TYPE_VOICE_ENGINE = 0,
    FEATURE_TYPE_EXPRESSION_ENGINE,
    FEATURE_TYPE_INTERACTION_ENGINE,
    FEATURE_TYPE_TRANSLATION,
    FEATURE_TYPE_CHAT,
    FEATURE_TYPE_CUSTOM_BASE = 1000  // Base for custom features
} feature_type_t;

/**
 * @brief Feature state
 */
typedef enum {
    FEATURE_STATE_DISABLED = 0,
    FEATURE_STATE_ENABLED,
    FEATURE_STATE_ACTIVE,
    FEATURE_STATE_ERROR
} feature_state_t;

/**
 * @brief Feature event types
 */
typedef enum {
    FEATURE_EVENT_ENABLED = 0,
    FEATURE_EVENT_DISABLED,
    FEATURE_EVENT_ACTIVATED,
    FEATURE_EVENT_DEACTIVATED,
    FEATURE_EVENT_ERROR,
    FEATURE_EVENT_DATA_READY,
    FEATURE_EVENT_CUSTOM_BASE = 1000  // Base for custom events
} feature_event_type_t;

/**
 * @brief Feature event data structure
 */
typedef struct {
    feature_event_type_t type;
    feature_type_t feature_type;
    void *data;
    uint32_t data_size;
} feature_event_t;

/**
 * @brief Feature event callback function prototype
 */
typedef void (*feature_event_callback_t)(feature_event_t *event, void *user_data);

/**
 * @brief Voice engine configuration
 */
typedef struct {
    uint32_t sample_rate;
    uint8_t channels;
    bool enable_vad;  // Voice Activity Detection
    bool enable_keyword_detection;
    char **keywords;
    uint32_t keyword_count;
    float detection_threshold;
    bool enable_noise_suppression;
    bool enable_echo_cancellation;
    bool offline_mode;         // Enable offline processing
    uint32_t local_cache_size; // Size of local cache in KB
    bool prioritize_local;     // Prioritize local processing over cloud
} voice_engine_config_t;

/**
 * @brief Expression engine configuration
 */
typedef struct {
    bool enable_animations;
    uint32_t animation_fps;
    bool enable_sound_effects;
    bool enable_haptic_feedback;
    bool enable_led_indicators;
    uint8_t brightness_level;
} expression_engine_config_t;

/**
 * @brief Interaction engine configuration
 */
typedef struct {
    bool enable_touch_detection;
    bool enable_gesture_detection;
    bool enable_proximity_detection;
    float touch_sensitivity;
    float gesture_sensitivity;
    uint32_t proximity_threshold_cm;
} interaction_engine_config_t;

/**
 * @brief Translation engine configuration
 */
typedef struct {
    char *default_source_language;
    char *default_target_language;
    bool enable_auto_language_detection;
    bool enable_offline_mode;
    uint32_t cache_size_kb;
    bool enable_continuous_translation;
    uint32_t max_offline_phrases;     // Maximum number of cached phrases
    float cache_confidence_threshold;  // Threshold for using cached translations
    bool prioritize_offline;          // Prefer offline translations when possible
} translation_engine_config_t;

/**
 * @brief Chat engine configuration
 */
typedef struct {
    char *bot_name;
    char *personality_profile;
    bool enable_context_awareness;
    uint32_t context_history_size;
    bool enable_offline_responses;
    uint32_t response_timeout_ms;
    uint32_t offline_cache_size;  // Size of offline response cache
    uint32_t max_offline_entries; // Maximum number of cached responses
    float offline_confidence_threshold; // Threshold for using cached responses
} chat_engine_config_t;

/**
 * @brief Feature Manager configuration
 */
typedef struct {
    bool auto_start_features;
    uint32_t max_concurrent_features;
    uint32_t feature_task_stack_size;
    sys_task_priority_t feature_task_priority;
} feature_manager_config_t;

/**
 * @brief Initialize the Feature Manager
 * 
 * @param config Pointer to Feature Manager configuration
 * @return feature_status_t Initialization status
 */
feature_status_t feature_manager_init(feature_manager_config_t *config);

/**
 * @brief Deinitialize the Feature Manager
 * 
 * @return feature_status_t Deinitialization status
 */
feature_status_t feature_manager_deinit(void);

/**
 * @brief Register a feature event callback
 * 
 * @param feature_type Feature type to register for (or -1 for all features)
 * @param event_type Event type to register for (or -1 for all events)
 * @param callback Callback function
 * @param user_data User data to pass to the callback
 * @return feature_status_t Status code
 */
feature_status_t feature_register_callback(feature_type_t feature_type,
                                          feature_event_type_t event_type,
                                          feature_event_callback_t callback,
                                          void *user_data);

/**
 * @brief Unregister a feature event callback
 * 
 * @param feature_type Feature type
 * @param event_type Event type
 * @param callback Callback function to unregister
 * @return feature_status_t Status code
 */
feature_status_t feature_unregister_callback(feature_type_t feature_type,
                                            feature_event_type_t event_type,
                                            feature_event_callback_t callback);

/**
 * @brief Enable a feature
 * 
 * @param feature_type Feature type to enable
 * @return feature_status_t Status code
 */
feature_status_t feature_enable(feature_type_t feature_type);

/**
 * @brief Disable a feature
 * 
 * @param feature_type Feature type to disable
 * @return feature_status_t Status code
 */
feature_status_t feature_disable(feature_type_t feature_type);

/**
 * @brief Get the state of a feature
 * 
 * @param feature_type Feature type
 * @param state Pointer to store the feature state
 * @return feature_status_t Status code
 */
feature_status_t feature_get_state(feature_type_t feature_type, feature_state_t *state);

/**
 * @brief Initialize the Voice Engine
 * 
 * @param config Pointer to Voice Engine configuration
 * @return feature_status_t Initialization status
 */
feature_status_t voice_engine_init(voice_engine_config_t *config);

/**
 * @brief Start voice recording
 * 
 * @return feature_status_t Status code
 */
feature_status_t voice_engine_start_recording(void);

/**
 * @brief Stop voice recording
 * 
 * @return feature_status_t Status code
 */
feature_status_t voice_engine_stop_recording(void);

/**
 * @brief Play audio data
 * 
 * @param data Audio data buffer
 * @param size Size of audio data in bytes
 * @return feature_status_t Status code
 */
feature_status_t voice_engine_play_audio(const uint8_t *data, uint32_t size);

/**
 * @brief Initialize the Expression Engine
 * 
 * @param config Pointer to Expression Engine configuration
 * @return feature_status_t Initialization status
 */
feature_status_t expression_engine_init(expression_engine_config_t *config);

/**
 * @brief Play an animation
 * 
 * @param animation_name Name of the animation to play
 * @param loop Whether to loop the animation
 * @return feature_status_t Status code
 */
feature_status_t expression_engine_play_animation(const char *animation_name, bool loop);

/**
 * @brief Stop the current animation
 * 
 * @return feature_status_t Status code
 */
feature_status_t expression_engine_stop_animation(void);

/**
 * @brief Play a sound effect
 * 
 * @param sound_name Name of the sound effect to play
 * @return feature_status_t Status code
 */
feature_status_t expression_engine_play_sound(const char *sound_name);

/**
 * @brief Initialize the Interaction Engine
 * 
 * @param config Pointer to Interaction Engine configuration
 * @return feature_status_t Initialization status
 */
feature_status_t interaction_engine_init(interaction_engine_config_t *config);

/**
 * @brief Enable touch detection
 * 
 * @param enable Whether to enable touch detection
 * @return feature_status_t Status code
 */
feature_status_t interaction_engine_enable_touch(bool enable);

/**
 * @brief Enable gesture detection
 * 
 * @param enable Whether to enable gesture detection
 * @return feature_status_t Status code
 */
feature_status_t interaction_engine_enable_gestures(bool enable);

/**
 * @brief Initialize the Translation Engine
 * 
 * @param config Pointer to Translation Engine configuration
 * @return feature_status_t Initialization status
 */
feature_status_t translation_engine_init(translation_engine_config_t *config);

/**
 * @brief Translate text
 * 
 * @param text Text to translate
 * @param source_language Source language (or NULL for auto-detection)
 * @param target_language Target language
 * @param translated_text Buffer to store translated text
 * @param max_size Maximum size of the translated text buffer
 * @return feature_status_t Status code
 */
feature_status_t translation_engine_translate_text(const char *text,
                                                 const char *source_language,
                                                 const char *target_language,
                                                 char *translated_text,
                                                 uint32_t max_size);

/**
 * @brief Initialize the Chat Engine
 * 
 * @param config Pointer to Chat Engine configuration
 * @return feature_status_t Initialization status
 */
feature_status_t chat_engine_init(chat_engine_config_t *config);

/**
 * @brief Send a message to the chat engine
 * 
 * @param message Message text
 * @param context_id Context ID (or NULL for default context)
 * @param response Buffer to store the response
 * @param max_size Maximum size of the response buffer
 * @return feature_status_t Status code
 */
feature_status_t chat_engine_send_message(const char *message,
                                        const char *context_id,
                                        char *response,
                                        uint32_t max_size);

#endif /* TOFU_FEATURE_MANAGER_H */
