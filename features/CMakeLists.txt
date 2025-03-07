# Features CMakeLists.txt

# Language Detection module
target_sources(tofu_features PRIVATE
    language_detection/language_detection.c
)

# Translation Cache Layer module
target_sources(tofu_features PRIVATE
    translation_engine/translation_cache_layer.c
)

target_include_directories(tofu_features PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/language_detection
    ${CMAKE_CURRENT_SOURCE_DIR}/translation_engine
)
