cmake_minimum_required(VERSION 3.14)

project(kolosal-cli
  LANGUAGES CXX
  VERSION 1.0.0
  DESCRIPTION "Kolosal CLI - A Command Line Interface to use, deploy, and manage LLMs locally"
)

# Include UCM for runtime library management
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)

# Static link the runtime libraries
ucm_set_runtime(DYNAMIC)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(USE_CUDA "Enable CUDA support" OFF)
option(USE_VULKAN "Enable Vulkan support" OFF)
option(USE_MPI "Enable MPI support" OFF)
option(ENABLE_NATIVE_OPTIMIZATION "Enable native CPU optimization (-march=native)" OFF)

# Propagate options to subprojects
set(USE_CUDA ${USE_CUDA} CACHE BOOL "Enable CUDA support" FORCE)
set(USE_VULKAN ${USE_VULKAN} CACHE BOOL "Enable Vulkan support" FORCE)
set(USE_MPI ${USE_MPI} CACHE BOOL "Enable MPI support" FORCE)

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

# Also depend on the inference target - get the name from the inference build
if(USE_CUDA)
    set(INFERENCE_TARGET_NAME "llama-cuda")
elseif(USE_VULKAN)
    set(INFERENCE_TARGET_NAME "llama-vulkan")
else()
    set(INFERENCE_TARGET_NAME "llama-cpu")
endif()

# Add dependency on the inference target
add_dependencies(kolosal ${INFERENCE_TARGET_NAME})

# Link libraries
target_link_libraries(kolosal ${CURL_LIBRARY})

# Link yaml-cpp library from kolosal-server
target_link_libraries(kolosal yaml-cpp)

# Platform-specific linking and post-build steps
if(WIN32)
    # Windows-specific libraries
    target_link_libraries(kolosal ws2_32 wldap32 crypt32 normaliz)
    
    # Copy curl DLL to output directory - try multiple possible locations
    set(CURL_DLL_PATHS
        "${CURL_DIR}/bin/libcurl.dll"
        "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/libcurl.dll"
        "${CMAKE_CURRENT_BINARY_DIR}/kolosal-server/bin/$<CONFIG>/libcurl.dll"
    )
    
    # Function to copy DLL if it exists
    function(copy_dll_if_exists target_name source_path dest_name comment_text)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${source_path}"
            "$<TARGET_FILE_DIR:${target_name}>/${dest_name}"
            COMMENT "${comment_text}"
            VERBATIM
        )
    endfunction()
    
    # Copy curl DLL from primary source
    copy_dll_if_exists(kolosal "${CURL_DIR}/bin/libcurl.dll" "libcurl.dll" "Copying libcurl.dll to CLI output directory")
    
    # Copy Kolosal server DLL
    copy_dll_if_exists(kolosal "$<TARGET_FILE:kolosal_server>" "kolosal_server.dll" "Copying kolosal_server.dll to CLI output directory")
    
    # Copy Kolosal server executable
    copy_dll_if_exists(kolosal "$<TARGET_FILE:kolosal_server_exe>" "kolosal-server.exe" "Copying kolosal-server.exe to CLI output directory")
    
    # Copy the inference engine DLL from the built location
    copy_dll_if_exists(kolosal "$<TARGET_FILE:${INFERENCE_TARGET_NAME}>" "${INFERENCE_TARGET_NAME}.dll" "Copying ${INFERENCE_TARGET_NAME}.dll to CLI output directory")
else()
    # Linux-specific libraries
    target_link_libraries(kolosal pthread dl)
    
    # Link filesystem library for C++17 std::filesystem on older GCC versions
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
        target_link_libraries(kolosal stdc++fs)
        message(STATUS "Linking with stdc++fs for std::filesystem support on GCC < 9.0")
    endif()
    
    # On Linux, check for required system libraries
    find_package(OpenSSL)
    if(OpenSSL_FOUND)
        target_link_libraries(kolosal OpenSSL::SSL OpenSSL::Crypto)
        message(STATUS "Found OpenSSL: ${OPENSSL_VERSION}")
    else()
        message(WARNING "OpenSSL not found. HTTPS features might not work properly. Install libssl-dev (Ubuntu/Debian) or openssl-devel (RHEL/CentOS)")
    endif()
    
    find_package(ZLIB)
    if(ZLIB_FOUND)
        target_link_libraries(kolosal ZLIB::ZLIB)
        message(STATUS "Found zlib: ${ZLIB_VERSION_STRING}")
    else()
        message(WARNING "zlib not found. Install zlib1g-dev (Ubuntu/Debian) or zlib-devel (RHEL/CentOS)")
    endif()
    
    # Add Linux-specific compile definitions
    target_compile_definitions(kolosal PRIVATE 
        _GNU_SOURCE
        __LINUX__
    )
    
    # Enable large file support
    target_compile_definitions(kolosal PRIVATE
        _FILE_OFFSET_BITS=64
        _LARGEFILE_SOURCE
        _LARGEFILE64_SOURCE
    )
    
    # Apply optimization flags for Release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR NOT CMAKE_BUILD_TYPE)
        if(ENABLE_NATIVE_OPTIMIZATION)
            target_compile_options(kolosal PRIVATE -march=native -mtune=native)
            message(STATUS "Native CPU optimization enabled for kolosal")
        else()
            target_compile_options(kolosal PRIVATE -O3)
            message(STATUS "Using safe optimization flags for kolosal")
        endif()
    endif()
    
    # Copy Kolosal server shared library with RPATH setting
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:kolosal_server>"
        "$<TARGET_FILE_DIR:kolosal>/libkolosal_server.so"
        COMMENT "Copying libkolosal_server.so to CLI output directory"
    )
    
    # Copy Kolosal server executable
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:kolosal_server_exe>"
        "$<TARGET_FILE_DIR:kolosal>/kolosal-server"
        COMMENT "Copying kolosal-server to CLI output directory"
    )
    
    # Copy the inference engine shared library
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${INFERENCE_TARGET_NAME}>"
        "$<TARGET_FILE_DIR:kolosal>/lib${INFERENCE_TARGET_NAME}.so"
        COMMENT "Copying lib${INFERENCE_TARGET_NAME}.so to CLI output directory"
    )
    
    # Set RPATH for runtime library search in the same directory
    set_target_properties(kolosal PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endif()

# Copy config.yaml to output directory for all platforms
add_custom_command(TARGET kolosal POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/config.yaml"
    $<TARGET_FILE_DIR:kolosal>/config.yaml)

# Installation rules
if(WIN32)
    # Install the main executable to bin subdirectory for proper PATH handling
    install(TARGETS kolosal
        RUNTIME DESTINATION bin
        COMPONENT Runtime
    )
    
    # Install the curl DLL to bin subdirectory
    install(FILES "${CURL_DIR}/bin/libcurl.dll"
        DESTINATION bin
        COMPONENT Runtime
    )
    
    # Install the kolosal server files to bin subdirectory
    install(FILES 
        $<TARGET_FILE:kolosal_server>
        $<TARGET_FILE:kolosal_server_exe>
        DESTINATION bin
        COMPONENT Runtime
    )
    
    # Install the inference DLL 
    install(FILES 
        $<TARGET_FILE:${INFERENCE_TARGET_NAME}>
        DESTINATION bin
        COMPONENT Runtime
    )
    
    # Install configuration file to the root directory
    install(FILES config.yaml
        DESTINATION .
        COMPONENT Runtime
    )
    
elseif(UNIX)
    # Install the main executable (now named kolosal)
    install(TARGETS kolosal
        RUNTIME DESTINATION bin
    )
    
    # Install the kolosal server executable and library
    install(TARGETS kolosal_server_exe kolosal_server ${INFERENCE_TARGET_NAME}
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
# Platform-specific generator selection
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
else()
    set(CPACK_GENERATOR "DEB")
endif()

set(CPACK_PACKAGE_NAME "kolosal")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Kolosal CLI - Command Line Interface for LLM Management")
set(CPACK_PACKAGE_DESCRIPTION "A Command Line Interface to use, deploy, and manage Large Language Models (LLMs) locally with the Kolosal framework.")
set(CPACK_PACKAGE_VENDOR "Kolosal Project")
set(CPACK_PACKAGE_CONTACT "dev@kolosal.com")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/kolosal/kolosal-cli")

# Windows-specific NSIS settings
if(WIN32)
    set(CPACK_NSIS_DISPLAY_NAME "Kolosal CLI")
    set(CPACK_NSIS_PACKAGE_NAME "Kolosal CLI")
    set(CPACK_NSIS_HELP_LINK "https://github.com/kolosal/kolosal-cli")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/kolosal/kolosal-cli")
    set(CPACK_NSIS_CONTACT "dev@kolosal.com")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    
    # Disable built-in PATH modification since we handle it with custom commands
    set(CPACK_NSIS_MODIFY_PATH OFF)
    
    # Set installation directory - use LocalAppData for user-level install
    set(CPACK_NSIS_INSTALL_ROOT "$LOCALAPPDATA")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Kolosal")
    
    # Custom NSIS configuration to override AddToPath function
    set(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS "
        ; These commands are executed before the main section
        !include \\\"WinMessages.nsh\\\"
    ")
    
    # Override the built-in AddToPath functionality by providing custom code
    # that executes before the problematic built-in function is called
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
        ; Override the built-in PATH modification with PowerShell approach
        ; This executes within the installation section
        StrCmp \\\$DO_NOT_ADD_TO_PATH \\\"1\\\" skip_custom_path
        
        ; Use PowerShell to add to PATH (handles long paths)
        DetailPrint \\\"Adding \$INSTDIR\\\\bin to user PATH using PowerShell...\\\"
        nsExec::ExecToLog 'powershell.exe -ExecutionPolicy Bypass -Command \\\"\$currentPath = [Environment]::GetEnvironmentVariable('PATH', 'User'); if (\$currentPath -and \$currentPath -notlike '*\$INSTDIR\\\\bin*') { \$newPath = \$currentPath + ';\$INSTDIR\\\\bin'; [Environment]::SetEnvironmentVariable('PATH', \$newPath, 'User') } elseif (-not \$currentPath) { [Environment]::SetEnvironmentVariable('PATH', '\$INSTDIR\\\\bin', 'User') }\\\"'
        Pop \$0
        IntCmp \$0 0 ps_path_success ps_path_fallback ps_path_fallback
        
        ps_path_fallback:
        DetailPrint \\\"PowerShell failed, using registry fallback...\\\"
        ReadRegStr \$1 HKCU \\\"Environment\\\" \\\"PATH\\\"
        StrCmp \$1 \\\"\\\" 0 +3
        WriteRegExpandStr HKCU \\\"Environment\\\" \\\"PATH\\\" \\\"\$INSTDIR\\\\bin\\\"
        Goto ps_path_success
        StrCpy \$2 \\\"\$1;\$INSTDIR\\\\bin\\\"
        WriteRegExpandStr HKCU \\\"Environment\\\" \\\"PATH\\\" \\\"\$2\\\"
        
        ps_path_success:
        DetailPrint \\\"PATH updated successfully\\\"
        
        skip_custom_path:
        
        ; Create App Paths entry for easier command line access
        WriteRegStr HKCU \\\"SOFTWARE\\\\Microsoft\\\\Windows\\\\CurrentVersion\\\\App Paths\\\\kolosal.exe\\\" \\\"\\\" \\\"\$INSTDIR\\\\bin\\\\kolosal.exe\\\"
        WriteRegStr HKCU \\\"SOFTWARE\\\\Microsoft\\\\Windows\\\\CurrentVersion\\\\App Paths\\\\kolosal.exe\\\" \\\"Path\\\" \\\"\$INSTDIR\\\\bin\\\"
        
        ; Broadcast environment change to notify all applications
        SendMessage \\\${HWND_BROADCAST} \\\${WM_WININICHANGE} 0 \\\"STR:Environment\\\" /TIMEOUT=5000
    ")
    
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
        ; Remove from PATH using PowerShell
        StrCmp \\\$DO_NOT_ADD_TO_PATH \\\"1\\\" skip_custom_unpath
        
        DetailPrint \\\"Removing \$INSTDIR\\\\bin from user PATH using PowerShell...\\\"
        nsExec::ExecToLog 'powershell.exe -ExecutionPolicy Bypass -Command \\\"\$currentPath = [Environment]::GetEnvironmentVariable('PATH', 'User'); if (\$currentPath) { \$pathEntries = \$currentPath -split ';' | Where-Object { \$_.Trim() -ne '\$INSTDIR\\\\bin' -and \$_.Trim() -ne '' }; \$newPath = \$pathEntries -join ';'; [Environment]::SetEnvironmentVariable('PATH', \$newPath, 'User') }\\\"'
        Pop \$0
        
        skip_custom_unpath:
        
        ; Remove App Paths entry
        DeleteRegKey HKCU \\\"SOFTWARE\\\\Microsoft\\\\Windows\\\\CurrentVersion\\\\App Paths\\\\kolosal.exe\\\"
        
        ; Broadcast environment change to notify all applications
        SendMessage \\\${HWND_BROADCAST} \\\${WM_WININICHANGE} 0 \\\"STR:Environment\\\" /TIMEOUT=5000
    ")
    
    # Add custom install and uninstall commands with PowerShell-based PATH handling

    
    # Menu shortcuts
    set(CPACK_PACKAGE_EXECUTABLES "kolosal" "Kolosal CLI")
    set(CPACK_CREATE_DESKTOP_LINKS "kolosal")
    
    # License file (optional)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    endif()
    
    # Installation components for Windows
    set(CPACK_COMPONENTS_ALL Runtime)
    set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Kolosal CLI Runtime")
    set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Main executable and dependencies")
    set(CPACK_COMPONENT_RUNTIME_REQUIRED TRUE)
    
    # Architecture detection for Windows
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CPACK_SYSTEM_NAME "win64")
    else()
        set(CPACK_SYSTEM_NAME "win32")
    endif()
else()
    # DEB package specific settings (Linux)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Kolosal Project <dev@kolosal.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    
    # More comprehensive dependencies for better compatibility
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.17), libgcc-s1 (>= 3.0), libstdc++6 (>= 9)")
    
    # Add SSL/TLS support dependencies
    if(OpenSSL_FOUND)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libssl3 (>= 3.0.0) | libssl1.1 (>= 1.1.0)")
    endif()
    
    # Add CURL dependencies
    if(CURL_FOUND)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libcurl4 (>= 7.16.2)")
    endif()
    
    # Add compression support
    if(ZLIB_FOUND)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, zlib1g (>= 1:1.1.4)")
    endif()
    
    # Add threading and dynamic linking support
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libpthread-stubs0-dev | libc6-dev")
    
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    
    # Post-install script to set up library paths and permissions
    set(POSTINST_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/postinst")
    file(WRITE ${POSTINST_SCRIPT}
"#!/bin/bash
# Post-installation script for Kolosal CLI

# Ensure the binary has execute permissions
chmod +x /usr/bin/kolosal
chmod +x /usr/bin/kolosal-server

# Update library cache
if command -v ldconfig >/dev/null 2>&1; then
    ldconfig
fi

# Create symlinks for shared libraries if they don't exist in standard locations
if [ -f /usr/lib/libkolosal_server.so ] && [ ! -e /usr/lib/x86_64-linux-gnu/libkolosal_server.so ]; then
    ln -sf /usr/lib/libkolosal_server.so /usr/lib/x86_64-linux-gnu/libkolosal_server.so 2>/dev/null || true
fi

# Notify user about configuration
echo \"Kolosal CLI installed successfully!\"
echo \"Configuration file located at: /etc/kolosal/config.yaml\"
echo \"Run 'kolosal --help' to get started.\"

exit 0
")
    
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${POSTINST_SCRIPT}")
    
    # Ensure the package installs files correctly
    set(CPACK_COMPONENTS_ALL Runtime)
    set(CPACK_DEB_COMPONENT_INSTALL OFF)
    
    # Architecture detection for Linux
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
    else()
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "all")
    endif()
    
    # Add RPM support for RHEL/CentOS/Fedora
    set(CPACK_RPM_PACKAGE_NAME "kolosal-cli")
    set(CPACK_RPM_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_RPM_PACKAGE_RELEASE "1")
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Tools")
    set(CPACK_RPM_PACKAGE_VENDOR "Kolosal Project")
    set(CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")
    set(CPACK_RPM_PACKAGE_REQUIRES "glibc >= 2.17, gcc-c++, libcurl, openssl-libs, zlib")
    
    # Check if we should build RPM instead of DEB
    if(EXISTS "/etc/redhat-release" OR EXISTS "/etc/fedora-release")
        set(CPACK_GENERATOR "RPM")
        message(STATUS "Detected RedHat-based system - will generate RPM packages")
    else()
        message(STATUS "Will generate DEB packages (use -DCPACK_GENERATOR=RPM to force RPM)")
    endif()
endif()

include(CPack)

if(WIN32)
    message(STATUS "CPack configured for NSIS package generation")
    message(STATUS "  Run 'cmake --build . --target package' to create the Windows installer")
    message(STATUS "  The installer will install to user profile and add to user PATH")
else()
    if(CPACK_GENERATOR STREQUAL "RPM")
        message(STATUS "CPack configured for RPM package generation")
        message(STATUS "  Run 'cmake --build . --target package' to create the RPM installer")
        message(STATUS "  Install with: sudo rpm -i kolosal-cli-*.rpm")
    else()
        message(STATUS "CPack configured for DEB package generation")
        message(STATUS "  Run 'cmake --build . --target package' to create the DEB installer")
        message(STATUS "  Install with: sudo dpkg -i kolosal-cli-*.deb")
    endif()
    message(STATUS "  Or use: make package")
    message(STATUS "")
    message(STATUS "Linux Build Requirements:")
    message(STATUS "  - libcurl4-openssl-dev (Ubuntu/Debian) or libcurl-devel (RHEL/CentOS)")
    message(STATUS "  - libssl-dev (Ubuntu/Debian) or openssl-devel (RHEL/CentOS)")
    message(STATUS "  - zlib1g-dev (Ubuntu/Debian) or zlib-devel (RHEL/CentOS)")
    message(STATUS "  - build-essential cmake (Ubuntu/Debian) or gcc-c++ cmake (RHEL/CentOS)")
endif()