cmake_minimum_required(VERSION 3.14)

project(kolosal-cli
  LANGUAGES CXX
  VERSION 1.0.0
  DESCRIPTION "Kolosal CLI - A Command Line Interface to use, deploy, and manage LLMs locally"
)

# Include UCM for runtime library management
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)

# Static link the runtime libraries
ucm_set_runtime(STATIC)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Print UCM flags for debugging
ucm_print_flags()

# Add the kolosal-server submodule
add_subdirectory(kolosal-server)

# Add external libraries
set(CURL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/curl)
set(NLOHMANN_JSON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)

# Include directories
if(WIN32)
    include_directories(${CURL_DIR}/include)
else()
    # On Linux, use system CURL
    find_package(CURL REQUIRED)
    if(CURL_FOUND)
        include_directories(${CURL_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "CURL not found. Please install libcurl4-openssl-dev (Ubuntu/Debian) or libcurl-devel (RHEL/CentOS)")
    endif()
endif()

include_directories(${NLOHMANN_JSON_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/kolosal-server/external/yaml-cpp/include)

# Find curl library
if(WIN32)
    find_library(CURL_LIBRARY
        NAMES curl libcurl
        PATHS ${CURL_DIR}/lib
        NO_DEFAULT_PATH
    )
else()
    # On Linux, CURL_LIBRARIES is already set by find_package
    set(CURL_LIBRARY ${CURL_LIBRARIES})
endif()

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
add_executable(kolosal ${SOURCES} ${HEADERS})

# Make kolosal depend on the server targets to ensure proper build order
add_dependencies(kolosal kolosal_server_exe kolosal_server)

# Link libraries
target_link_libraries(kolosal ${CURL_LIBRARY})

# Link yaml-cpp library from kolosal-server
target_link_libraries(kolosal yaml-cpp)

# Platform-specific linking and post-build steps
if(WIN32)
    # Windows-specific libraries
    target_link_libraries(kolosal ws2_32 wldap32 crypt32 normaliz)
    
    # Copy curl DLL to output directory
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CURL_DIR}/bin/libcurl.dll"
        $<TARGET_FILE_DIR:kolosal>)
    
    # Copy Kolosal server files from built submodule to output directory
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:kolosal_server>
        $<TARGET_FILE_DIR:kolosal>/kolosal_server.dll)
    
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:kolosal_server_exe>
        $<TARGET_FILE_DIR:kolosal>/kolosal-server.exe)
else()
    # Linux-specific libraries
    target_link_libraries(kolosal pthread dl)
    
    # On Linux, check for required system libraries
    find_package(OpenSSL)
    if(OpenSSL_FOUND)
        target_link_libraries(kolosal OpenSSL::SSL OpenSSL::Crypto)
    endif()
    
    find_package(ZLIB)
    if(ZLIB_FOUND)
        target_link_libraries(kolosal ZLIB::ZLIB)
    endif()
    
    # Copy Kolosal server files from built submodule to output directory
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:kolosal_server>
        $<TARGET_FILE_DIR:kolosal>/libkolosal_server.so)
    
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:kolosal_server_exe>
        $<TARGET_FILE_DIR:kolosal>/kolosal-server)
endif()

# Copy config.yaml to output directory for all platforms
add_custom_command(TARGET kolosal POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/config.yaml"
    $<TARGET_FILE_DIR:kolosal>/config.yaml)

# Installation rules
if(UNIX)
    # Install the main executable (now named kolosal)
    install(TARGETS kolosal
        RUNTIME DESTINATION bin
    )
    
    # Install the kolosal server executable and library
    install(TARGETS kolosal_server_exe kolosal_server
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
    )
    
    # Install configuration file
    install(FILES config.yaml
        DESTINATION etc/kolosal
    )
    
    # Create a desktop entry (optional)
    set(DESKTOP_FILE "${CMAKE_CURRENT_BINARY_DIR}/kolosal-cli.desktop")
    file(WRITE ${DESKTOP_FILE}
"[Desktop Entry]
Version=1.0
Type=Application
Name=Kolosal CLI
Comment=Command Line Interface for Kolosal LLM Management
Exec=kolosal
Icon=kolosal
Terminal=true
Categories=Development;Science;
")
    install(FILES ${DESKTOP_FILE}
        DESTINATION share/applications
        OPTIONAL
    )
endif()

# Print build summary
message(STATUS "")
message(STATUS "=====================================================")
message(STATUS "  Kolosal CLI Build Configuration Summary")
message(STATUS "=====================================================")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
if(WIN32)
    message(STATUS "  CURL library: ${CURL_LIBRARY}")
else()
    message(STATUS "  CURL libraries: ${CURL_LIBRARIES}")
endif()
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "=====================================================")
message(STATUS "")

# CPack configuration for packaging
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "kolosal")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Kolosal CLI - Command Line Interface for LLM Management")
set(CPACK_PACKAGE_DESCRIPTION "A Command Line Interface to use, deploy, and manage Large Language Models (LLMs) locally with the Kolosal framework.")
set(CPACK_PACKAGE_VENDOR "Kolosal Project")
set(CPACK_PACKAGE_CONTACT "dev@kolosal.com")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/kolosal/kolosal-cli")

# DEB package specific settings
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Kolosal Project <dev@kolosal.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.17), libssl3 (>= 3.0.0), libcurl4 (>= 7.16.2), zlib1g (>= 1:1.1.4)")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

# Ensure the package installs files correctly
set(CPACK_COMPONENTS_ALL Runtime)
set(CPACK_DEB_COMPONENT_INSTALL OFF)

# Architecture detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
else()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "all")
endif()

include(CPack)

message(STATUS "CPack configured for DEB package generation")
message(STATUS "  Run 'make package' to create the installer")