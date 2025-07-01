cmake_minimum_required(VERSION 3.14)

project(kolosal-cli
  LANGUAGES CXX
  VERSION 1.0.0
  DESCRIPTION "Kolosal CLI - A Command Line Interface to use, deploy, and manage LLMs locally"
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
add_executable(kolosal-cli ${SOURCES} ${HEADERS})

# Make kolosal-cli depend on the server targets to ensure proper build order
add_dependencies(kolosal-cli kolosal_server_exe kolosal_server)

# Link libraries
target_link_libraries(kolosal-cli ${CURL_LIBRARY})

# Link yaml-cpp library from kolosal-server
target_link_libraries(kolosal-cli yaml-cpp)

# Platform-specific linking and post-build steps
if(WIN32)
    # Windows-specific libraries
    target_link_libraries(kolosal-cli ws2_32 wldap32 crypt32 normaliz)
    
    # Copy curl DLL to output directory
    add_custom_command(TARGET kolosal-cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CURL_DIR}/bin/libcurl.dll"
        $<TARGET_FILE_DIR:kolosal-cli>)
    
    # Copy Kolosal server files from built submodule to output directory
    add_custom_command(TARGET kolosal-cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:kolosal_server>
        $<TARGET_FILE_DIR:kolosal-cli>/kolosal_server.dll)
    
    add_custom_command(TARGET kolosal-cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:kolosal_server_exe>
        $<TARGET_FILE_DIR:kolosal-cli>/kolosal-server.exe)
else()
    # Linux-specific libraries
    target_link_libraries(kolosal-cli pthread dl)
    
    # On Linux, check for required system libraries
    find_package(OpenSSL)
    if(OpenSSL_FOUND)
        target_link_libraries(kolosal-cli OpenSSL::SSL OpenSSL::Crypto)
    endif()
    
    find_package(ZLIB)
    if(ZLIB_FOUND)
        target_link_libraries(kolosal-cli ZLIB::ZLIB)
    endif()
    
    # Copy Kolosal server files from built submodule to output directory
    add_custom_command(TARGET kolosal-cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:kolosal_server>
        $<TARGET_FILE_DIR:kolosal-cli>/libkolosal_server.so)
    
    add_custom_command(TARGET kolosal-cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:kolosal_server_exe>
        $<TARGET_FILE_DIR:kolosal-cli>/kolosal-server)
endif()

# Copy config.yaml to output directory for all platforms
add_custom_command(TARGET kolosal-cli POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/config.yaml"
    $<TARGET_FILE_DIR:kolosal-cli>/config.yaml)

# Installation rules
if(UNIX)
    # Install the main executable
    install(TARGETS kolosal-cli 
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
Exec=kolosal-cli
Icon=kolosal-cli
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