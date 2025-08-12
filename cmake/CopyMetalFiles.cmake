# CMake script to find and copy Metal GPU files for macOS packaging
# This script searches for Metal-related files in the kolosal-server build directory
# and copies them to the destination directory for packaging

set(METAL_FILES
    "ggml-common.h"
    "ggml-metal-impl.h"
    "ggml-metal.metal"
)

# Define search directories (in order of preference)
set(SEARCH_DIRS
    "${SOURCE_DIR}"
    # New location for llama.cpp (under inference)
    "${SOURCE_DIR}/inference/external/llama.cpp"
    "${SOURCE_DIR}/inference"
    "${SOURCE_DIR}/src"
    # Legacy fallback location (pre-move)
    "${SOURCE_DIR}/external/llama.cpp"
    "${SOURCE_DIR}/inference/external/llama.cpp/src"
    "${SOURCE_DIR}/external/llama.cpp/src"
)

message(STATUS "Searching for Metal GPU files...")
message(STATUS "Source directory: ${SOURCE_DIR}")
message(STATUS "Destination directory: ${DEST_DIR}")

# Ensure destination directory exists
file(MAKE_DIRECTORY "${DEST_DIR}")

# Search for each Metal file
foreach(METAL_FILE ${METAL_FILES})
    set(FILE_FOUND FALSE)
    
    foreach(SEARCH_DIR ${SEARCH_DIRS})
        set(SOURCE_PATH "${SEARCH_DIR}/${METAL_FILE}")
        
        if(EXISTS "${SOURCE_PATH}")
            message(STATUS "Found ${METAL_FILE} at: ${SOURCE_PATH}")
            
            # Copy the file to destination
            file(COPY "${SOURCE_PATH}" DESTINATION "${DEST_DIR}")
            message(STATUS "Copied ${METAL_FILE} to ${DEST_DIR}")
            
            set(FILE_FOUND TRUE)
            break()
        endif()
    endforeach()
    
    if(NOT FILE_FOUND)
        message(WARNING "Metal file not found: ${METAL_FILE}")
        message(STATUS "Searched in directories:")
        foreach(SEARCH_DIR ${SEARCH_DIRS})
            message(STATUS "  - ${SEARCH_DIR}")
        endforeach()
    endif()
endforeach()

# Also search for any additional Metal-related files using wildcards
file(GLOB_RECURSE ADDITIONAL_METAL_FILES 
    "${SOURCE_DIR}/**/ggml*.h"
    "${SOURCE_DIR}/**/ggml*.metal"
)

message(STATUS "Found additional Metal files:")
foreach(METAL_FILE ${ADDITIONAL_METAL_FILES})
    get_filename_component(FILENAME "${METAL_FILE}" NAME)
    
    # Only copy files we haven't already copied
    if(NOT EXISTS "${DEST_DIR}/${FILENAME}")
        message(STATUS "Copying additional file: ${FILENAME}")
        file(COPY "${METAL_FILE}" DESTINATION "${DEST_DIR}")
    endif()
endforeach()

message(STATUS "Metal GPU files copy completed.")
