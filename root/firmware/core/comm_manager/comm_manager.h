/**
 * @file comm_manager.h
 * @brief Communication Manager for To-fu device
 * 
 * This file defines the Communication Manager interface for the To-fu device.
 * The Communication Manager is responsible for handling all communication
 * interfaces, including WiFi, BLE, and protocol handling for cloud services
 * and device-to-device communication.
 */

#ifndef TOFU_COMM_MANAGER_H
#define TOFU_COMM_MANAGER_H

#include "firmware_config.h"
#include "hal.h"
#include "system_manager.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Communication Manager status codes
 */
typedef enum {
    COMM_STATUS_OK = 0,
    COMM_STATUS_ERROR_GENERAL,
    COMM_STATUS_ERROR_NOT_INITIALIZED,
    COMM_STATUS_ERROR_ALREADY_INITIALIZED,
    COMM_STATUS_ERROR_INVALID_PARAM,
    COMM_STATUS_ERROR_NOT_CONNECTED,
    COMM_STATUS_ERROR_CONNECTION_FAILED,
    COMM_STATUS_ERROR_TIMEOUT,
    COMM_STATUS_ERROR_AUTHENTICATION,
    COMM_STATUS_ERROR_PROTOCOL,
    COMM_STATUS_ERROR_BUSY,
    COMM_STATUS_ERROR_MEMORY,
    COMM_STATUS_ERROR_CACHE_FULL,      // Added for cache operations
    COMM_STATUS_ERROR_CACHE_INVALID    // Added for cache operations
} comm_status_t;

/**
 * @brief Communication interface types
 */
typedef enum {
    COMM_INTERFACE_WIFI = 0,
    COMM_INTERFACE_BLE,
    COMM_INTERFACE_CLOUD,
    COMM_INTERFACE_LEADER_FOLLOWER,
    COMM_INTERFACE_LOCAL_CACHE         // Added for local cache interface
} comm_interface_t;

/**
 * @brief Communication connection state
 */
typedef enum {
    COMM_STATE_DISCONNECTED = 0,
    COMM_STATE_CONNECTING,
    COMM_STATE_CONNECTED,
    COMM_STATE_DISCONNECTING,
    COMM_STATE_ERROR,
    COMM_STATE_OFFLINE_MODE           // Added for offline operation
} comm_state_t;

/**
 * @brief Communication event types
 */
typedef enum {
    COMM_EVENT_CONNECTED = 0,
    COMM_EVENT_DISCONNECTED,
    COMM_EVENT_CONNECTION_FAILED,
    COMM_EVENT_DATA_RECEIVED,
    COMM_EVENT_DATA_SENT,
    COMM_EVENT_ERROR,
    COMM_EVENT_LEADER_FOUND,
    COMM_EVENT_FOLLOWER_FOUND,
    COMM_EVENT_LEADER_LOST,
    COMM_EVENT_FOLLOWER_LOST,
    COMM_EVENT_CLOUD_AUTHENTICATED,
    COMM_EVENT_CLOUD_AUTH_FAILED,
    COMM_EVENT_CACHE_HIT,            // Added for cache operations
    COMM_EVENT_CACHE_MISS,           // Added for cache operations
    COMM_EVENT_CACHE_UPDATED,        // Added for cache operations
    COMM_EVENT_OFFLINE_MODE_ENTERED, // Added for offline operation
    COMM_EVENT_OFFLINE_MODE_EXITED   // Added for offline operation
} comm_event_type_t;

/**
 * @brief Communication event data structure
 */
typedef struct {
    comm_event_type_t type;
    comm_interface_t interface;
    void *data;
    uint32_t data_size;
    uint32_t timestamp;
} comm_event_t;

/**
 * @brief Communication event callback function prototype
 */
typedef void (*comm_event_callback_t)(comm_event_t *event, void *user_data);

/**
 * @brief WiFi configuration
 */
typedef struct {
    char *ssid;
    char *password;
    bool use_static_ip;
    uint8_t ip_addr[4];
    uint8_t gateway[4];
    uint8_t netmask[4];
    uint8_t dns[4];
    uint32_t connection_timeout_ms;
    uint8_t max_retry_count;
    bool enable_power_save;          // Added for power management
    bool auto_reconnect;             // Added for connection management
} wifi_config_t;

/**
 * @brief BLE configuration
 */
typedef struct {
    char *device_name;
    uint8_t max_connections;
    bool is_discoverable;
    bool is_connectable;
    uint32_t advertising_interval_ms;
    uint8_t tx_power_level;         // 0-7, higher is stronger
    bool enable_power_save;         // Added for power management
    uint32_t connection_interval;   // Added for connection optimization
} ble_config_t;

/**
 * @brief Cloud service configuration
 */
typedef struct {
    char *server_url;
    uint16_t server_port;
    bool use_ssl;
    char *api_key;
    char *device_id;
    uint32_t connection_timeout_ms;
    uint32_t keep_alive_interval_ms;
    uint32_t reconnect_interval_ms;
    uint8_t max_retry_count;
    bool enable_offline_mode;       // Added for offline support
    bool prioritize_local_cache;    // Added for cache prioritization
    uint32_t offline_cache_size;    // Added for offline storage
    float cache_hit_threshold;      // Added for cache confidence
    uint32_t sync_interval_ms;      // Added for background syncing
} cloud_config_t;

/**
 * @brief Leader-Follower configuration
 */
typedef struct {
    tofu_device_role_t role;
    char *leader_id;
    uint8_t max_followers;
    uint32_t discovery_timeout_ms;
    uint32_t connection_timeout_ms;
    uint32_t keep_alive_interval_ms;
    bool enable_offline_mode;       // Added for offline operation
    bool auto_failover;            // Added for reliability
    uint32_t offline_buffer_size;  // Added for offline storage
    bool prioritize_local;         // Added for local processing
} leader_follower_config_t;

/**
 * @brief Communication Manager configuration
 */
typedef struct {
    bool auto_connect_wifi;
    bool auto_connect_cloud;
    bool auto_setup_leader_follower;
    uint32_t max_packet_size;
    uint32_t rx_buffer_size;
    uint32_t tx_buffer_size;
    uint32_t max_queued_messages;
    uint32_t offline_buffer_size;    // Added: Size of offline storage buffer
    bool enable_local_cache;         // Added: Enable local response caching
    bool prioritize_offline;         // Added: Prefer offline operation when possible
    uint32_t cache_cleanup_interval; // Added: Interval to cleanup old cached data
    float cache_hit_threshold;       // Added: Confidence threshold for cache hits
    bool enable_power_save;          // Added: Power saving features
    bool enable_auto_failover;       // Added: Automatic failover to offline mode
} comm_manager_config_t;

/**
 * @brief Message priority levels
 */
typedef enum {
    COMM_PRIORITY_LOW = 0,
    COMM_PRIORITY_NORMAL,
    COMM_PRIORITY_HIGH,
    COMM_PRIORITY_CRITICAL,
    COMM_PRIORITY_OFFLINE           // Added for offline processing
} comm_priority_t;

/**
 * @brief Message structure
 */
typedef struct {
    uint32_t message_id;
    comm_interface_t interface;
    comm_priority_t priority;
    void *data;
    uint32_t data_size;
    uint32_t timestamp;
    bool require_ack;
    uint32_t timeout_ms;
    void (*callback)(uint32_t message_id, bool success, void *user_data);
    void *user_data;
    bool allow_cache;              // Added: Allow caching this message
    bool force_offline;            // Added: Force offline processing
} comm_message_t;

// Function declarations
comm_status_t comm_manager_init(comm_manager_config_t *config);
comm_status_t comm_manager_deinit(void);
comm_status_t comm_register_callback(comm_interface_t interface,
                                    comm_event_type_t event_type,
                                    comm_event_callback_t callback,
                                    void *user_data);
comm_status_t comm_unregister_callback(comm_interface_t interface,
                                      comm_event_type_t event_type,
                                      comm_event_callback_t callback);
comm_status_t wifi_init(wifi_config_t *config);
comm_status_t wifi_connect(void);
comm_status_t wifi_disconnect(void);
comm_status_t wifi_get_state(comm_state_t *state);
comm_status_t wifi_get_signal_strength(int8_t *rssi);
comm_status_t ble_init(ble_config_t *config);
comm_status_t ble_start_advertising(void);
comm_status_t ble_stop_advertising(void);
comm_status_t ble_get_state(comm_state_t *state);
comm_status_t ble_send_data(const uint8_t *data, uint32_t size, uint8_t connection_id);
comm_status_t cloud_init(cloud_config_t *config);
comm_status_t cloud_connect(void);
comm_status_t cloud_disconnect(void);
comm_status_t cloud_get_state(comm_state_t *state);
comm_status_t cloud_send_data(const char *endpoint,
                             const uint8_t *data,
                             uint32_t size,
                             uint8_t *response_buffer,
                             uint32_t response_size,
                             uint32_t *actual_response_size);
comm_status_t leader_follower_init(leader_follower_config_t *config);
comm_status_t leader_follower_start_discovery(void);
comm_status_t leader_follower_stop_discovery(void);
comm_status_t leader_follower_connect(const char *device_id);
comm_status_t leader_follower_disconnect(const char *device_id);
comm_status_t leader_follower_send_data(const char *device_id,
                                       const uint8_t *data,
                                       uint32_t size);
comm_status_t leader_follower_get_connected_devices(char **device_ids,
                                                  uint8_t max_count,
                                                  uint8_t *actual_count);
comm_status_t comm_send_message(comm_message_t *message);
comm_status_t comm_create_message(comm_interface_t interface,
                                 const uint8_t *data,
                                 uint32_t data_size,
                                 comm_priority_t priority,
                                 comm_message_t *message);

// Cache management functions
comm_status_t comm_cache_init(uint32_t size_kb);
comm_status_t comm_cache_store(const char *key, const uint8_t *data, uint32_t size);
comm_status_t comm_cache_retrieve(const char *key, uint8_t *buffer, uint32_t size, uint32_t *actual_size);
comm_status_t comm_is_offline(bool *is_offline);
comm_status_t comm_force_offline(bool enable);

#endif /* TOFU_COMM_MANAGER_H */
