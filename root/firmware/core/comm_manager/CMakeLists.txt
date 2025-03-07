# Communication component CMakeLists.txt

# Set component sources
set(COMPONENT_SRCS
    "wifi_stack.c"
    "ble_stack.c"
    "cloud_client.c"
    "leader_follower.c"
    "protocol_handler.c"
)

# Set component include directories
set(COMPONENT_ADD_INCLUDEDIRS
    "."
)

# Set component requirements
set(COMPONENT_REQUIRES
    driver
    esp_wifi
    bt
    nvs_flash
    esp_http_client
    mqtt
    json
)

# Register the component
register_component()
