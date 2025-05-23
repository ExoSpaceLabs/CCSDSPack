set(ENCODER_EXEC "ccsds_encoder")

message(STATUS "Building: ${ENCODER_EXEC}")

# Collect all source files for the encoder executable
set(ENCODER_SOURCES
    "${SOURCE_DIR}/exec_encoder.cpp"
    "${SOURCE_DIR}/exec_utils.cpp"
)
set(DECODER_HEADERS
    "${INCLUDE_DIR}/exec_utils.h"
)
# Create the encoder executable target
add_executable(${ENCODER_EXEC} ${ENCODER_SOURCES} ${DECODER_HEADERS})

# Add include directories to the encoder target
target_include_directories(${ENCODER_EXEC} PRIVATE ${INCLUDE_DIR})

# Link the encoder executable with the library
target_link_libraries(${ENCODER_EXEC} PRIVATE ${LIB_NAME})

# Set the output directory for the encoder executable
set_target_properties(${ENCODER_EXEC} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIR}          # Specifies where the executable is placed
)

# Platform-specific RPATH settings
if(UNIX)
    # Linux or macOS
    set_target_properties(${ENCODER_EXEC} PROPERTIES
            BUILD_RPATH "/usr/local/lib:${LIBRARY_OUTPUT_DIR}"  # During build time, look in these paths for libraries
            INSTALL_RPATH "/usr/local/lib:${LIBRARY_OUTPUT_DIR}" # After installation, look in these paths for libraries
    )
elseif(WIN32)
    # Windows doesn't use RPATH. DLLs are usually placed in the same directory as the EXE or specified in PATH.
    set_target_properties(${ENCODER_EXEC} PROPERTIES
            # No need to set RPATH on Windows
            RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIR}  # Ensure DLL is next to the executable
    )
endif()
