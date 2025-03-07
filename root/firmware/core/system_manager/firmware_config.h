/**
 * @file firmware_config.h
 * @brief Main configuration file for To-fu firmware
 * 
 * This file contains the primary configuration settings for the To-fu
 * device firmware, including hardware pins, feature flags, and system
 * parameters.
 */

#ifndef TOFU_FIRMWARE_CONFIG_H
#define TOFU_FIRMWARE_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Hardware version information
 */
#define TOFU_HW_VERSION_MAJOR 0
#define TOFU_HW_VERSION_MINOR 1
#define TOFU_HW_VERSION_PATCH 0

/**
 * @brief Firmware version information
 */
#define TOFU_FW_VERSION_MAJOR 0
#define TOFU_FW_VERSION_MINOR 1
#define TOFU_FW_VERSION_PATCH 0

/**
 * @brief Device role configuration
 */
typedef enum {
    TOFU_ROLE_FOLLOWER = 0,  // Changed to make Follower the default role
    TOFU_ROLE_LEADER = 1
} tofu_device_role_t;

/**
 * @brief ESP32 Pin Definitions
 */
// Audio Module
#define PIN_I2S_BCLK        26
#define PIN_I2S_LRCLK       25
#define PIN_I2S_DOUT        22
#define PIN_I2S_DIN         21
#define PIN_AUDIO_ENABLE    19

// SD Card Interface
#define PIN_SPI_MISO        12
#define PIN_SPI_MOSI        13
#define PIN_SPI_CLK         14
#define PIN_SD_CS           15

// PDM Microphone
#define PIN_PDM_CLK         32
#define PIN_PDM_DATA        33

// Display Interface
#define PIN_I2C_SDA         18
#define PIN_I2C_SCL         5

// Power Management
#define PIN_BATT_LEVEL      34
#define PIN_POWER_ENABLE    27

/**
 * @brief System Configuration
 */
// System Configuration - Optimized for Follower role
#define TOFU_SYSTEM_TICK_MS           10
#define TOFU_WATCHDOG_TIMEOUT_MS      5000
#define TOFU_MAX_TASK_PRIORITY        5
#define TOFU_LOCAL_CACHE_SIZE_KB      512  // Local cache for offline operation
#define TOFU_MAX_OFFLINE_PHRASES      100   // Number of cached responses

/**
 * @brief Memory Configuration - Increased for better local processing
 */
#define TOFU_HEAP_SIZE_KB             256  // Increased for local processing
#define TOFU_STACK_SIZE_BYTES         8192 // Increased for complex tasks

/**
 * @brief Communication Configuration
 */
#define TOFU_WIFI_RETRY_COUNT         5
#define TOFU_WIFI_RETRY_INTERVAL_MS   5000
#define TOFU_BLE_DEVICE_NAME          "To-fu"
#define TOFU_BLE_MAX_CONNECTIONS      3

/**
 * @brief Feature Configuration - Prioritized for Follower role
 */
#define TOFU_FEATURE_VOICE_ENGINE     1     // Priority: Voice processing
#define TOFU_FEATURE_LOCAL_LANG       1     // Priority: Local language detection
#define TOFU_FEATURE_OFFLINE_MODE     1     // Priority: Offline operation
#define TOFU_FEATURE_BATTERY_MONITOR  1     // Priority: Power management
#define TOFU_FEATURE_EXPRESSION       0     // Future: Expression capabilities
#define TOFU_FEATURE_CLOUD_SERVICES   0     // Future: Advanced cloud features

/**
 * @brief Debug Configuration
 */
#define TOFU_DEBUG_ENABLED            1
#define TOFU_DEBUG_LEVEL              3  // 0=OFF, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=VERBOSE

#endif /* TOFU_FIRMWARE_CONFIG_H */
