/**
 * @file main.c
 * @brief Main entry point for To-fu device firmware
 * 
 * This file contains the main entry point for the To-fu device firmware.
 * It initializes all the necessary components and starts the system.
 */

#include "firmware_config.h"
#include "hal.h"
#include "system_manager.h"
#include "feature_manager.h"
#include "comm_manager.h"
#include <stdio.h>
#include <string.h>

// Tag for logging
#define TAG "MAIN"

// Forward declarations
static void system_event_handler(sys_event_t *event, void *user_data);
static void feature_event_handler(feature_event_t *event, void *user_data);
static void comm_event_handler(comm_event_t *event, void *user_data);

/**
 * @brief Application entry point
 */
void app_main(void)
{
    hal_status_t hal_status;
    sys_status_t sys_status;
    feature_status_t feature_status;
    comm_status_t comm_status;
    
    // Initialize HAL
    hal_status = hal_init();
    if (hal_status != HAL_STATUS_OK) {
        printf("HAL initialization failed with status: %d\n", hal_status);
        return;
    }
    
    // Configure system - Optimized for Follower role
    sys_config_t sys_config = {
        .device_role = TOFU_ROLE_FOLLOWER,  // Set as follower by default
        .enable_watchdog = true,
        .watchdog_timeout_ms = TOFU_WATCHDOG_TIMEOUT_MS,
        .max_events_queued = 32,
        .max_tasks = 10,
        .auto_start_features = true,
        .auto_connect_wifi = false,  // Don't auto-connect, wait for app pairing
        .auto_connect_cloud = false  // Cloud connection not prioritized
    };
    
    // Initialize system manager
    sys_status = sys_init(&sys_config);
    if (sys_status != SYS_STATUS_OK) {
        printf("System initialization failed with status: %d\n", sys_status);
        hal_deinit();
        return;
    }
    
    // Register system event handler
    sys_status = sys_event_register_callback(SYS_EVENT_NONE, system_event_handler, NULL);
    if (sys_status != SYS_STATUS_OK) {
        printf("Failed to register system event handler: %d\n", sys_status);
    }
    
    // Configure feature manager
    feature_manager_config_t feature_config = {
        .auto_start_features = true,
        .max_concurrent_features = 5,
        .feature_task_stack_size = TOFU_STACK_SIZE_BYTES,
        .feature_task_priority = SYS_TASK_PRIORITY_NORMAL
    };
    
    // Initialize feature manager
    feature_status = feature_manager_init(&feature_config);
    if (feature_status != FEATURE_STATUS_OK) {
        printf("Feature manager initialization failed with status: %d\n", feature_status);
        sys_deinit();
        hal_deinit();
        return;
    }
    
    // Register feature event handler
    feature_status = feature_register_callback((feature_type_t)-1, (feature_event_type_t)-1, 
                                              feature_event_handler, NULL);
    if (feature_status != FEATURE_STATUS_OK) {
        printf("Failed to register feature event handler: %d\n", feature_status);
    }
    
    // Configure voice engine - Optimized for local processing
    voice_engine_config_t voice_config = {
        .sample_rate = 16000,
        .channels = 1,
        .enable_vad = true,
        .enable_keyword_detection = true,
        .keywords = NULL,  // Will be set based on local language support
        .keyword_count = 0,
        .detection_threshold = 0.7f,
        .enable_noise_suppression = true,
        .enable_echo_cancellation = true,
        .offline_mode = true,              // Prioritize offline processing
        .local_cache_size = TOFU_LOCAL_CACHE_SIZE_KB,
        .prioritize_local = true          // Prefer local processing
    };
    
    // Initialize voice engine
    feature_status = voice_engine_init(&voice_config);
    if (feature_status != FEATURE_STATUS_OK) {
        printf("Voice engine initialization failed with status: %d\n", feature_status);
    }
    
    // Initialize cache system first
    comm_status_t cache_status = comm_cache_init(TOFU_LOCAL_CACHE_SIZE_KB);
    if (cache_status != COMM_STATUS_OK) {
        printf("Cache initialization failed with status: %d\n", cache_status);
    }

    // Configure communication manager - Optimized for Follower role with priority on offline operation
    comm_manager_config_t comm_config = {
        .auto_connect_wifi = false,         // Manual connection via app
        .auto_connect_cloud = false,        // Cloud connection not prioritized
        .auto_setup_leader_follower = false,// Manual setup
        .max_packet_size = 1024,
        .rx_buffer_size = 8192,            // Increased for better local buffering
        .tx_buffer_size = 8192,            // Increased for better local buffering
        .max_queued_messages = 64,         // Increased queue size
        .offline_buffer_size = TOFU_LOCAL_CACHE_SIZE_KB,
        .enable_local_cache = true,        // Enable local caching
        .prioritize_offline = true,        // Prefer offline operation
        .cache_cleanup_interval = 3600000, // Clean cache every hour
        .cache_hit_threshold = 0.8f,      // High confidence for cache hits
        .enable_power_save = true,        // Enable power saving
        .enable_auto_failover = true      // Auto switch to offline mode
    };
    
    // Initialize communication manager
    comm_status = comm_manager_init(&comm_config);
    if (comm_status != COMM_STATUS_OK) {
        printf("Communication manager initialization failed with status: %d\n", comm_status);
        feature_manager_deinit();
        sys_deinit();
        hal_deinit();
        return;
    }
    
    // Register communication event handler
    comm_status = comm_register_callback((comm_interface_t)-1, (comm_event_type_t)-1, 
                                        comm_event_handler, NULL);
    if (comm_status != COMM_STATUS_OK) {
        printf("Failed to register communication event handler: %d\n", comm_status);
    }
    
    // Initialize local language detection with cache support
    translation_engine_config_t translation_config = {
        .default_source_language = NULL,     // Auto-detect
        .default_target_language = NULL,     // Set during pairing
        .enable_auto_language_detection = true,
        .enable_offline_mode = true,
        .cache_size_kb = TOFU_LOCAL_CACHE_SIZE_KB,
        .enable_continuous_translation = true,
        .max_offline_phrases = TOFU_MAX_OFFLINE_PHRASES,
        .cache_confidence_threshold = 0.8f,  // High confidence for cached translations
        .prioritize_offline = true          // Prefer offline translations
    };

    feature_status = translation_engine_init(&translation_config);
    if (feature_status != FEATURE_STATUS_OK) {
        printf("Translation engine initialization failed with status: %d\n", feature_status);
    }
    
    // Configure leader-follower - Follower specific with offline support
    leader_follower_config_t lf_config = {
        .role = TOFU_ROLE_FOLLOWER,
        .leader_id = NULL,             // Will be set during pairing
        .max_followers = 0,            // Follower doesn't accept connections
        .discovery_timeout_ms = 30000,
        .connection_timeout_ms = 10000,
        .keep_alive_interval_ms = 5000,
        .enable_offline_mode = true,   // Enable offline operation
        .auto_failover = true,         // Automatic failover when disconnected
        .offline_buffer_size = TOFU_LOCAL_CACHE_SIZE_KB,
        .prioritize_local = true       // Prefer local processing
    };
    
    // Initialize leader-follower
    comm_status = leader_follower_init(&lf_config);
    if (comm_status != COMM_STATUS_OK) {
        printf("Leader-follower initialization failed with status: %d\n", comm_status);
    }
    
    // Start the system
    sys_log(3, TAG, "Starting To-fu system...");
    sys_status = sys_start();
    if (sys_status != SYS_STATUS_OK) {
        printf("Failed to start system: %d\n", sys_status);
        comm_manager_deinit();
        feature_manager_deinit();
        sys_deinit();
        hal_deinit();
        return;
    }
    
    sys_log(3, TAG, "To-fu system started successfully");
    
    // The system is now running in the background
    // The main thread can now perform other tasks or enter a sleep loop
    
    while (1) {
        // Main loop - can be used for background tasks or monitoring
        // Most functionality is handled by the system manager and its tasks
        
        // Get system statistics
        sys_stats_t stats;
        sys_get_stats(&stats);
        
        // Log system status periodically
        sys_log(4, TAG, "System uptime: %u ms, Free heap: %u bytes, CPU usage: %u%%",
                stats.uptime_ms, stats.free_heap, stats.cpu_usage_percent);
        
        // Sleep for a while
        sys_task_delay(10000);  // 10 seconds
    }
}

/**
 * @brief System event handler
 * 
 * @param event System event
 * @param user_data User data (unused)
 */
static void system_event_handler(sys_event_t *event, void *user_data)
{
    (void)user_data;  // Unused parameter
    
    switch (event->type) {
        case SYS_EVENT_BOOT_COMPLETE:
            sys_log(3, TAG, "System boot complete");
            break;
            
        case SYS_EVENT_ERROR:
            sys_log(1, TAG, "System error occurred");
            break;
            
        case SYS_EVENT_LOW_BATTERY:
            sys_log(2, TAG, "Low battery warning");
            break;
            
        case SYS_EVENT_WIFI_CONNECTED:
            sys_log(3, TAG, "WiFi connected");
            break;
            
        case SYS_EVENT_WIFI_DISCONNECTED:
            sys_log(2, TAG, "WiFi disconnected");
            break;
            
        case SYS_EVENT_CLOUD_CONNECTED:
            sys_log(3, TAG, "Cloud service connected");
            break;
            
        case SYS_EVENT_CLOUD_DISCONNECTED:
            sys_log(2, TAG, "Cloud service disconnected");
            break;
            
        case SYS_EVENT_UPDATE_AVAILABLE:
            sys_log(3, TAG, "Firmware update available");
            break;
            
        default:
            // Handle other events
            break;
    }
}

/**
 * @brief Feature event handler
 * 
 * @param event Feature event
 * @param user_data User data (unused)
 */
static void feature_event_handler(feature_event_t *event, void *user_data)
{
    (void)user_data;  // Unused parameter
    
    // Get feature type name for logging
    const char *feature_name = "Unknown";
    switch (event->feature_type) {
        case FEATURE_TYPE_VOICE_ENGINE:
            feature_name = "Voice Engine";
            break;
        case FEATURE_TYPE_EXPRESSION_ENGINE:
            feature_name = "Expression Engine";
            break;
        case FEATURE_TYPE_INTERACTION_ENGINE:
            feature_name = "Interaction Engine";
            break;
        case FEATURE_TYPE_TRANSLATION:
            feature_name = "Translation";
            break;
        case FEATURE_TYPE_CHAT:
            feature_name = "Chat";
            break;
        default:
            break;
    }
    
    switch (event->type) {
        case FEATURE_EVENT_ENABLED:
            sys_log(3, TAG, "Feature %s enabled", feature_name);
            break;
            
        case FEATURE_EVENT_DISABLED:
            sys_log(3, TAG, "Feature %s disabled", feature_name);
            break;
            
        case FEATURE_EVENT_ACTIVATED:
            sys_log(3, TAG, "Feature %s activated", feature_name);
            break;
            
        case FEATURE_EVENT_DEACTIVATED:
            sys_log(3, TAG, "Feature %s deactivated", feature_name);
            break;
            
        case FEATURE_EVENT_ERROR:
            sys_log(1, TAG, "Feature %s error", feature_name);
            break;
            
        case FEATURE_EVENT_DATA_READY:
            sys_log(4, TAG, "Feature %s data ready", feature_name);
            break;
            
        default:
            // Handle other events
            break;
    }
}

/**
 * @brief Communication event handler
 * 
 * @param event Communication event
 * @param user_data User data (unused)
 */
static void comm_event_handler(comm_event_t *event, void *user_data)
{
    (void)user_data;  // Unused parameter
    
    // Get interface name for logging
    const char *interface_name = "Unknown";
    switch (event->interface) {
        case COMM_INTERFACE_WIFI:
            interface_name = "WiFi";
            break;
        case COMM_INTERFACE_BLE:
            interface_name = "BLE";
            break;
        case COMM_INTERFACE_CLOUD:
            interface_name = "Cloud";
            break;
        case COMM_INTERFACE_LEADER_FOLLOWER:
            interface_name = "Leader-Follower";
            break;
        default:
            break;
    }
    
    switch (event->type) {
        case COMM_EVENT_CONNECTED:
            sys_log(3, TAG, "Interface %s connected", interface_name);
            break;
            
        case COMM_EVENT_DISCONNECTED:
            sys_log(3, TAG, "Interface %s disconnected", interface_name);
            break;
            
        case COMM_EVENT_CONNECTION_FAILED:
            sys_log(2, TAG, "Interface %s connection failed", interface_name);
            break;
            
        case COMM_EVENT_DATA_RECEIVED:
            sys_log(4, TAG, "Interface %s received %u bytes", interface_name, event->data_size);
            break;
            
        case COMM_EVENT_DATA_SENT:
            sys_log(4, TAG, "Interface %s sent data", interface_name);
            break;
            
        case COMM_EVENT_ERROR:
            sys_log(1, TAG, "Interface %s error", interface_name);
            break;
            
        case COMM_EVENT_LEADER_FOUND:
            sys_log(3, TAG, "Leader device found");
            break;
            
        case COMM_EVENT_FOLLOWER_FOUND:
            sys_log(3, TAG, "Follower device found");
            break;
            
        default:
            // Handle other events
            break;
    }
}
