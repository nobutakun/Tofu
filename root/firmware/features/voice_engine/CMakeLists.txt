# Voice Engine Component

set(COMPONENT_NAME "voice_engine")

# Source files
set(COMPONENT_SRCS
    "vad.c"
    "keyword_detect.c"
    "noise_suppress.c"
)

# Include directories
set(COMPONENT_ADD_INCLUDEDIRS
    "."              # Component's own directory
    "../.."         # Root directory for firmware_config.h
    "../../hal"     # HAL directory
)

# Register component
register_component()

# Component dependencies
set(COMPONENT_REQUIRES
    common_hal     # For hardware abstraction
    dsp           # For signal processing
)

# Compiler flags
target_compile_options(${COMPONENT_TARGET} PRIVATE
    -Wall
    -Werror
    -O2            # Optimization level
    -DFEATURE_VAD_ENABLED=1
    -DFEATURE_KWD_ENABLED=1
)

# Component compile definitions
target_compile_definitions(${COMPONENT_TARGET} PRIVATE
    MAX_FRAME_SIZE=512
    FEATURE_VECTOR_SIZE=13
    MAX_KEYWORDS=10
)
