cmake_minimum_required(VERSION 3.12)

project(kolosal-cli
  LANGUAGES CXX
  VERSION 1.0.0
  DESCRIPTION "Kolosal CLI - A Command Line Interface to use, deploy, and manage LLMs locally"
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add external libraries
set(CURL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/curl)
set(NLOHMANN_JSON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)

# Include directories
include_directories(${CURL_DIR}/include)
include_directories(${NLOHMANN_JSON_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Find curl library
find_library(CURL_LIBRARY
    NAMES curl libcurl
    PATHS ${CURL_DIR}/lib
    NO_DEFAULT_PATH
)

# Create directories if they don't exist
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Collect all source files
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
)

# Collect all header files (for IDE purposes)
file(GLOB_RECURSE HEADERS
    "include/*.h"
)

# Add executable
add_executable(kolosal-cli ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(kolosal-cli ${CURL_LIBRARY})

# On Windows, we might need additional libraries
if(WIN32)
    target_link_libraries(kolosal-cli ws2_32 wldap32 crypt32 normaliz)
    
    # Copy curl DLL to output directory
    add_custom_command(TARGET kolosal-cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CURL_DIR}/bin/libcurl.dll"
        $<TARGET_FILE_DIR:kolosal-cli>)
endif()