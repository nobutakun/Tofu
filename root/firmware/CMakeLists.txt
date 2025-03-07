# The minimum version of CMake required
cmake_minimum_required(VERSION 3.16)

# Include the ESP-IDF CMake build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Set the project name
project(tofu-firmware)

# Set component requirements
set(COMPONENT_REQUIRES
    driver              # Hardware drivers
    esp_wifi           # WiFi support
    bt                 # BLE support
    nvs_flash          # Non-volatile storage
    esp_http_client    # HTTP client
    mqtt               # MQTT protocol
    json               # JSON parsing
    spiffs            # SPI Flash File System
    fatfs             # FAT filesystem
    esp_https_ota      # OTA updates
    spi_flash         # Flash operations
    sdmmc             # SD card support
    esp_timer         # High resolution timer
    mdns              # For device discovery
)

# Set component sources - Prioritize local processing capabilities
set(COMPONENT_SRCS
    "main.c"
    "hal.c"
    "system_manager.c"
    "feature_manager.c"
    "comm_manager.c"
    # Core features
    "features/voice_engine.c"
    "features/voice_engine/vad.c"              # Voice activity detection
    "features/voice_engine/keyword_detect.c"    # Local keyword detection
    "features/voice_engine/noise_suppress.c"    # Noise suppression
    "features/interaction_engine.c"
    "features/translation_engine.c"
    "features/translation_engine/local_lang_detect.c"  # Local language detection
    "features/translation_engine/offline_trans.c"      # Offline translation
    "features/translation_engine/cache_manager.c"      # Translation caching
    "features/chat_engine.c"
    "features/chat_engine/offline_responses.c"         # Offline chat responses
    # Communication stacks
    "communication/wifi_stack.c"
    "communication/ble_stack.c"
    "communication/protocol_handler.c"
    "communication/leader_follower.c"
    "communication/cache_storage.c"            # Cache storage management
    # Optional cloud components
    "communication/cloud_client.c"
)

# Set component include directories
set(COMPONENT_ADD_INCLUDEDIRS
    "."
    "features"
    "features/voice_engine"
    "features/translation_engine"
    "features/chat_engine"
    "communication"
    "communication/cache"
)

# Register the component
register_component()

# Add compile options
target_compile_options(${COMPONENT_TARGET} PRIVATE
    -Wall
    -Werror
    -Wextra
    -Wno-unused-parameter
    -Wno-sign-compare
)

# Add compile definitions
target_compile_definitions(${COMPONENT_TARGET} PRIVATE
    CONFIG_TOFU_VERSION_MAJOR=0
    CONFIG_TOFU_VERSION_MINOR=1
    CONFIG_TOFU_VERSION_PATCH=0
    CONFIG_ENABLE_OFFLINE_MODE=1        # Enable offline operation
    CONFIG_PRIORITIZE_LOCAL=1           # Prioritize local processing
    CONFIG_ENABLE_LOCAL_CACHE=1         # Enable local caching
    CONFIG_MAX_CACHE_SIZE_KB=512        # Maximum cache size
    CONFIG_OFFLINE_RESPONSE_COUNT=100    # Number of offline responses
    CONFIG_MIN_CACHE_CONFIDENCE=80      # Minimum confidence for cache hits (%)
)

# Add subdirectories for components
add_subdirectory(features)
add_subdirectory(communication)

# Set build flags
idf_build_set_property(COMPILE_OPTIONS "-DLOG_LOCAL_LEVEL=ESP_LOG_INFO" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DCONFIG_LOG_MAXIMUM_LEVEL=3" APPEND)

# Set partition table
set(PARTITION_CSV_PATH "${CMAKE_CURRENT_SOURCE_DIR}/partitions.csv")
