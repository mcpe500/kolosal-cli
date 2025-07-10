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

# Add the inference engine as a separate target (built independently)
option(BUILD_INFERENCE_ENGINE "Build the inference engine separately" ON)
if(BUILD_INFERENCE_ENGINE)
    add_subdirectory(kolosal-server/inference)
    
    # Determine the inference target name based on acceleration options
    if(USE_CUDA)
        set(INFERENCE_TARGET_NAME "llama-cuda")
    elseif(USE_VULKAN)
        set(INFERENCE_TARGET_NAME "llama-vulkan")
    else()
        set(INFERENCE_TARGET_NAME "llama-cpu")
    endif()
    
    message(STATUS "Inference target name: ${INFERENCE_TARGET_NAME}")
endif()

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

# Note: Inference engine is now built separately and loaded at runtime
# No direct dependencies on inference targets

# Link libraries
target_link_libraries(kolosal ${CURL_LIBRARY})

# Link yaml-cpp library from kolosal-server
target_link_libraries(kolosal yaml-cpp)

# Platform-specific linking and post-build steps
if(WIN32)
    # Windows-specific libraries
    target_link_libraries(kolosal ws2_32 wldap32 crypt32 normaliz)
    
    # Set output directory for Windows
    set_target_properties(kolosal PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
    )
    
    # Function to copy DLL if it exists
    function(copy_dll_if_exists target_name source_path dest_name comment_text)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${source_path}"
            "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${dest_name}"
            COMMENT "${comment_text}"
            VERBATIM
        )
    endfunction()
    
    # Copy curl DLL from primary source
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CURL_DIR}/bin/libcurl.dll"
        "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/libcurl.dll"
        COMMENT "Copying libcurl.dll to output directory"
        VERBATIM
    )
    
    # Copy Kolosal server DLL
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:kolosal_server>"
        "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/"
        COMMENT "Copying kolosal_server.dll to output directory"
        DEPENDS kolosal_server
    )
    
    # Copy Kolosal server executable
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:kolosal_server_exe>"
        "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/"
        COMMENT "Copying kolosal-server.exe to output directory"
        DEPENDS kolosal_server_exe
    )
    
    # Copy the inference engine DLL
    if(BUILD_INFERENCE_ENGINE)
        add_custom_command(TARGET kolosal POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${INFERENCE_TARGET_NAME}>"
            "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/"
            COMMENT "Copying ${INFERENCE_TARGET_NAME}.dll to output directory"
            DEPENDS ${INFERENCE_TARGET_NAME}
        )
    endif()
    
    # Copy any additional DLLs that may be in the server output directory
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/kolosal-server/$<CONFIG>/libcurl.dll"
        "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/libcurl.dll"
        COMMENT "Copying libcurl.dll from server directory"
        VERBATIM
    )
else()
    # Linux-specific libraries
    target_link_libraries(kolosal pthread dl)
    
    # Set output directory for Linux
    set_target_properties(kolosal PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
    
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
    
    # Copy Kolosal server shared library
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:kolosal_server>"
        "${CMAKE_BINARY_DIR}/bin/libkolosal_server.so"
        COMMENT "Copying libkolosal_server.so to output directory"
        DEPENDS kolosal_server
    )
    
    # Copy Kolosal server executable
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:kolosal_server_exe>"
        "${CMAKE_BINARY_DIR}/bin/kolosal-server"
        COMMENT "Copying kolosal-server to output directory"
        DEPENDS kolosal_server_exe
    )
    
    # Copy the inference engine shared library
    if(BUILD_INFERENCE_ENGINE)
        add_custom_command(TARGET kolosal POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${INFERENCE_TARGET_NAME}>"
            "${CMAKE_BINARY_DIR}/bin/lib${INFERENCE_TARGET_NAME}.so"
            COMMENT "Copying lib${INFERENCE_TARGET_NAME}.so to output directory"
            DEPENDS ${INFERENCE_TARGET_NAME}
        )
    endif()
    
    # Set RPATH for runtime library search in the same directory and standard lib paths
    set_target_properties(kolosal PROPERTIES
        INSTALL_RPATH "$ORIGIN:/usr/lib"
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endif()

# Copy config.yaml to output directory for all platforms
if(WIN32)
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.yaml"
        "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/config.yaml"
        COMMENT "Copying config.yaml to output directory"
    )
else()
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.yaml"
        "${CMAKE_BINARY_DIR}/bin/config.yaml"
        COMMENT "Copying config.yaml to output directory"
    )
endif()

# Installation rules
if(WIN32)
    # Install the main executable to bin subdirectory for proper PATH handling
    install(TARGETS kolosal
        RUNTIME DESTINATION bin
        COMPONENT Runtime
    )
    
    # Install the kolosal server targets (executable and library)
    install(TARGETS kolosal_server_exe kolosal_server
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib
        COMPONENT Runtime
    )
    
    # Install the inference engine DLL
    if(BUILD_INFERENCE_ENGINE)
        install(TARGETS ${INFERENCE_TARGET_NAME}
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION bin
            ARCHIVE DESTINATION lib
            COMPONENT Runtime
        )
    endif()
    
    # Install CURL DLL if it exists
    if(EXISTS "${CURL_DIR}/bin/libcurl.dll")
        install(FILES "${CURL_DIR}/bin/libcurl.dll"
            DESTINATION bin
            COMPONENT Runtime
        )
    endif()
    
    # Install DLLs using FILES instead of CODE for more reliable packaging
    # These paths should be evaluated at install time to pick up the correct DLLs
    
    # First, install the DLLs using direct target file references (most reliable)
    install(FILES 
        "$<TARGET_FILE:kolosal_server>"
        DESTINATION bin
        COMPONENT Runtime
    )
    
    if(BUILD_INFERENCE_ENGINE)
        install(FILES 
            "$<TARGET_FILE:${INFERENCE_TARGET_NAME}>"
            DESTINATION bin
            COMPONENT Runtime
        )
    endif()
    
    # Install CURL DLL with multiple fallback paths
    set(CURL_DLL_CANDIDATES
        "${CURL_DIR}/bin/libcurl.dll"
        "${CMAKE_BINARY_DIR}/kolosal-server/$<CONFIG>/libcurl.dll"
        "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/libcurl.dll"
    )
    
    foreach(CURL_DLL_PATH ${CURL_DLL_CANDIDATES})
        install(FILES "${CURL_DLL_PATH}"
            DESTINATION bin
            COMPONENT Runtime
            OPTIONAL
        )
    endforeach()
    
    # Install any yaml-cpp DLL if it exists as a shared library
    if(BUILD_SHARED_LIBS)
        install(FILES "$<TARGET_FILE:yaml-cpp>"
            DESTINATION bin
            COMPONENT Runtime
            OPTIONAL
        )
    endif()
    
    # Install additional DLLs from build output directories
    install(CODE "
        # Installation-time code to copy additional DLLs
        message(STATUS \"Copying additional DLLs to installation directory...\")
        
        # Define build output directories where DLLs might be found
        set(BUILD_OUTPUT_DIRS
            \"${CMAKE_BINARY_DIR}/kolosal-server/Debug\"
            \"${CMAKE_BINARY_DIR}/kolosal-server/Release\"
            \"${CMAKE_BINARY_DIR}/bin/Debug\"
            \"${CMAKE_BINARY_DIR}/bin/Release\"
            \"${CURL_DIR}/bin\"
        )
        
        # Additional DLLs that might be needed
        set(ADDITIONAL_DLLS
            \"libcurl.dll\"
            \"yaml-cpp.dll\"
        )
        
        # Copy additional DLLs if found
        foreach(dll_name \${ADDITIONAL_DLLS})
            foreach(search_dir \${BUILD_OUTPUT_DIRS})
                set(dll_path \"\${search_dir}/\${dll_name}\")
                if(EXISTS \"\${dll_path}\")
                    message(STATUS \"Installing additional DLL: \${dll_name} from \${dll_path}\")
                    file(INSTALL \"\${dll_path}\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\")
                    break()
                endif()
            endforeach()
        endforeach()
        
        # Copy any DLLs that might have been copied to the main executable directory during build
        if(EXISTS \"${CMAKE_BINARY_DIR}/Debug\")
            file(GLOB DEBUG_DLLS \"${CMAKE_BINARY_DIR}/Debug/*.dll\")
            foreach(dll_path \${DEBUG_DLLS})
                get_filename_component(dll_name \"\${dll_path}\" NAME)
                string(TOLOWER \"\${dll_name}\" dll_lower)
                if(NOT dll_lower MATCHES \"^(kernel32|user32|advapi32|msvcrt|vcruntime|msvcp|api-ms-)\")
                    message(STATUS \"Installing debug DLL: \${dll_name}\")
                    file(INSTALL \"\${dll_path}\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\")
                endif()
            endforeach()
        endif()
        
        if(EXISTS \"${CMAKE_BINARY_DIR}/Release\")
            file(GLOB RELEASE_DLLS \"${CMAKE_BINARY_DIR}/Release/*.dll\")
            foreach(dll_path \${RELEASE_DLLS})
                get_filename_component(dll_name \"\${dll_path}\" NAME)
                string(TOLOWER \"\${dll_name}\" dll_lower)
                if(NOT dll_lower MATCHES \"^(kernel32|user32|advapi32|msvcrt|vcruntime|msvcp|api-ms-)\")
                    message(STATUS \"Installing release DLL: \${dll_name}\")
                    file(INSTALL \"\${dll_path}\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\")
                endif()
            endforeach()
        endif()
    ")
    
    # Install configuration file to the root directory
    install(FILES config.yaml
        DESTINATION .
        COMPONENT Runtime
    )
    
    # Create a custom target to collect all DLLs in one place for easier packaging
    if(BUILD_INFERENCE_ENGINE)
        add_custom_target(collect_dlls ALL
            DEPENDS kolosal kolosal_server kolosal_server_exe ${INFERENCE_TARGET_NAME}
            COMMENT "Collecting all DLLs for packaging"
        )
    else()
        add_custom_target(collect_dlls ALL
            DEPENDS kolosal kolosal_server kolosal_server_exe
            COMMENT "Collecting DLLs for packaging"
        )
    endif()
    
    # Install all files from the output directory (includes all copied DLLs and executables)
    install(DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/"
        DESTINATION bin
        COMPONENT Runtime
        FILES_MATCHING 
        PATTERN "*.dll"
        PATTERN "*.exe"
        PATTERN "config.yaml"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
    
elseif(UNIX)
    # Install the main executable (now named kolosal)
    install(TARGETS kolosal
        RUNTIME DESTINATION bin
    )
    
    # Install the kolosal server executable and library with proper destinations
    install(TARGETS kolosal_server_exe
        RUNTIME DESTINATION bin
    )
    
    install(TARGETS kolosal_server
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    
    # Install the inference engine shared library
    if(BUILD_INFERENCE_ENGINE)
        install(TARGETS ${INFERENCE_TARGET_NAME}
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
        )
    endif()
    
    # Install configuration file to etc/kolosal directory
    install(FILES config.yaml
        DESTINATION etc/kolosal
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )
    
    # Create the kolosal configuration directory during install
    install(DIRECTORY DESTINATION etc/kolosal
        DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
    
    # Install systemd service file for kolosal-server if it exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/kolosal-server/kolosal-server.service")
        install(FILES kolosal-server/kolosal-server.service
            DESTINATION etc/systemd/system
            OPTIONAL
        )
    endif()
    
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
    
    # Install man page if it exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/docs/kolosal.1")
        install(FILES docs/kolosal.1
            DESTINATION share/man/man1
            OPTIONAL
        )
    endif()
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
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/kolosalai/kolosal-cli")

# Windows-specific NSIS settings
if(WIN32)
    set(CPACK_NSIS_DISPLAY_NAME "Kolosal CLI")
    set(CPACK_NSIS_PACKAGE_NAME "Kolosal CLI")
    set(CPACK_NSIS_HELP_LINK "https://github.com/kolosalai/kolosal-cli")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/kolosalai/kolosal-cli")
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
    set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Main executable, server components, and all dependencies")
    set(CPACK_COMPONENT_RUNTIME_REQUIRED TRUE)
    
    # Add information about what gets installed
    set(CPACK_NSIS_PACKAGE_INFO "
This installer includes:
- Kolosal CLI executable (kolosal.exe)
- Kolosal Server executable (kolosal-server.exe)
- Kolosal Server library (kolosal_server.dll)
- Inference Engine library (${INFERENCE_TARGET_NAME}.dll)
- Required dependencies (libcurl.dll, etc.)
- Configuration file (config.yaml)

The installation will:
- Install to your user profile directory
- Add the bin directory to your user PATH
- Create App Paths registry entries for easier access
")
    
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
    else()
        # Add basic SSL support even if not found during build
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libssl3 | libssl1.1")
    endif()
    
    # Add CURL dependencies
    if(CURL_FOUND)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libcurl4 (>= 7.16.2)")
    else()
        # Add basic CURL support even if not found during build
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libcurl4")
    endif()
    
    # Add compression support
    if(ZLIB_FOUND)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, zlib1g (>= 1:1.1.4)")
    else()
        # Add basic zlib support even if not found during build
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, zlib1g")
    endif()
    
    # Add threading support - use libpthread0 which is more universally available
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libpthread0 | libc6")
    
    # Initialize suggests packages list
    set(DEBIAN_SUGGESTS_LIST "")
    
    # Add acceleration-specific dependencies
    if(USE_CUDA)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, nvidia-cuda-runtime | cuda-runtime")
        set(DEBIAN_SUGGESTS_LIST "nvidia-cuda-dev")
    elseif(USE_VULKAN)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libvulkan1")
        set(DEBIAN_SUGGESTS_LIST "vulkan-tools, vulkan-validationlayers")
    endif()
    
    # Add optional but recommended packages
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "ca-certificates")
    
    # Build suggests list properly
    if(DEBIAN_SUGGESTS_LIST)
        set(CPACK_DEBIAN_PACKAGE_SUGGESTS "${DEBIAN_SUGGESTS_LIST}, systemd")
    else()
        set(CPACK_DEBIAN_PACKAGE_SUGGESTS "systemd")
    endif()
    
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    
    # Post-install script to set up library paths and permissions
    set(POSTINST_SCRIPT_IN "${CMAKE_CURRENT_BINARY_DIR}/postinst.in")
    set(POSTINST_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/postinst")
    file(WRITE ${POSTINST_SCRIPT_IN}
"#!/bin/bash
# Post-installation script for Kolosal CLI

# Ensure the binaries have execute permissions
chmod +x /usr/bin/kolosal
chmod +x /usr/bin/kolosal-server

# Create the kolosal configuration directory if it doesn't exist
if [ ! -d /etc/kolosal ]; then
    mkdir -p /etc/kolosal
    chmod 755 /etc/kolosal
fi

# Ensure config file has proper permissions (if it was installed)
if [ -f /etc/kolosal/config.yaml ]; then
    chmod 644 /etc/kolosal/config.yaml
    echo \"✓ Configuration file installed successfully\"
else
    # If config file is missing, create a default one
    echo \"Warning: Configuration file not found at /etc/kolosal/config.yaml\"
    echo \"Creating default configuration file...\"
    
    cat > /etc/kolosal/config.yaml << 'EOF'
server:
  port: \"8080\"
  host: \"0.0.0.0\"
  allow_public_access: false
  allow_internet_access: false
  idle_timeout: 300

logging:
  level: \"INFO\"
  file: \"\"
  access_log: false
  quiet_mode: false
  show_request_details: true

auth:
  enabled: true
  require_api_key: false
  api_key_header: \"X-API-Key\"
  api_keys: []
  
  rate_limit:
    enabled: true
    max_requests: 100
    window_size: 60
    
  cors:
    enabled: true
    allow_credentials: false
    max_age: 86400
    allowed_origins: [\"*\"]
    allowed_methods: [\"GET\", \"POST\", \"OPTIONS\"]
    allowed_headers: [\"Content-Type\", \"Authorization\", \"X-API-Key\"]

models: []

features:
  health_check: true
  metrics: false
EOF
    chmod 644 /etc/kolosal/config.yaml
    echo \"✓ Created default configuration file\"
fi

# Update library cache to include the new shared libraries
if command -v ldconfig >/dev/null 2>&1; then
    echo \"Updating library cache...\"
    ldconfig
fi

# Create symbolic links for easier library discovery if needed
if [ -f /usr/lib/libkolosal_server.so ] && [ ! -e /usr/lib/x86_64-linux-gnu/libkolosal_server.so ]; then
    ln -sf /usr/lib/libkolosal_server.so /usr/lib/x86_64-linux-gnu/libkolosal_server.so 2>/dev/null || true
fi

# Handle inference engine library
INFERENCE_LIB=\"/usr/lib/lib@INFERENCE_TARGET_NAME@.so\"
if [ -f \"\$INFERENCE_LIB\" ] && [ ! -e \"/usr/lib/x86_64-linux-gnu/lib@INFERENCE_TARGET_NAME@.so\" ]; then
    ln -sf \"\$INFERENCE_LIB\" \"/usr/lib/x86_64-linux-gnu/lib@INFERENCE_TARGET_NAME@.so\" 2>/dev/null || true
fi

# Create kolosal user and group if they don't exist (for systemd service)
if ! id \"kolosal\" >/dev/null 2>&1; then
    useradd --system --home-dir /var/lib/kolosal --shell /bin/false kolosal || true
    echo \"  ✓ Created kolosal system user\"
fi

# Add current user to kolosal group if possible (for CLI access)
if [ \"\$SUDO_USER\" ]; then
    usermod -a -G kolosal \"\$SUDO_USER\" 2>/dev/null || true
fi

# Create kolosal user directory for models and configuration if it doesn't exist
if [ ! -d /var/lib/kolosal ]; then
    mkdir -p /var/lib/kolosal
    chmod 755 /var/lib/kolosal
fi

# Set ownership of the kolosal data directory to be accessible by all users
chmod 755 /var/lib/kolosal
# Make it writable by the kolosal group, readable by others
if id \"kolosal\" >/dev/null 2>&1; then
    chown kolosal:kolosal /var/lib/kolosal
    chmod 775 /var/lib/kolosal  # Group writable
fi

# Create models directory for user convenience
if [ ! -d /var/lib/kolosal/models ]; then
    mkdir -p /var/lib/kolosal/models
    chmod 755 /var/lib/kolosal/models
    if id \"kolosal\" >/dev/null 2>&1; then
        chown kolosal:kolosal /var/lib/kolosal/models
        chmod 775 /var/lib/kolosal/models  # Group writable
    fi
fi

# Also ensure any user can create files in these directories for CLI usage
chmod g+w /var/lib/kolosal 2>/dev/null || true
chmod g+w /var/lib/kolosal/models 2>/dev/null || true

# Set up systemd service if systemctl is available
if command -v systemctl >/dev/null 2>&1 && [ -f /etc/systemd/system/kolosal-server.service ]; then
    echo \"Setting up kolosal-server systemd service...\"
    systemctl daemon-reload
    
    # Enable the service to start on boot
    if systemctl enable kolosal-server 2>/dev/null; then
        echo \"✓ Service enabled to start on boot\"
    else
        echo \"! Could not enable service (run: sudo systemctl enable kolosal-server)\"
    fi
    
    # Try to start the service
    if systemctl start kolosal-server 2>/dev/null; then
        echo \"✓ Service started successfully\"
        echo \"  Check status: systemctl status kolosal-server\"
    else
        echo \"! Could not start service automatically\"
        echo \"  Start manually: sudo systemctl start kolosal-server\"
        echo \"  Check logs: journalctl -u kolosal-server -f\"
    fi
else
    echo \"Note: systemd not available or service file not found\"
fi

# Notify user about configuration and usage
echo \"\"
echo \"===============================================\"
echo \"  Kolosal CLI installed successfully!\"
echo \"===============================================\"
echo \"Configuration file: /etc/kolosal/config.yaml\"
echo \"Data directory: /var/lib/kolosal\"
echo \"Models directory: /var/lib/kolosal/models\"
echo \"\"
echo \"Verifying installation:\"
if [ -f /usr/bin/kolosal ]; then
    echo \"  ✓ kolosal CLI installed\"
else
    echo \"  ✗ kolosal CLI missing\"
fi
if [ -f /usr/bin/kolosal-server ]; then
    echo \"  ✓ kolosal-server installed\"
else
    echo \"  ✗ kolosal-server missing\"
fi
if [ -f /etc/kolosal/config.yaml ]; then
    echo \"  ✓ Configuration file installed\"
else
    echo \"  ✗ Configuration file missing (will create default)\"
fi
if [ -f /usr/lib/libkolosal_server.so ]; then
    echo \"  ✓ Server library installed\"
else
    echo \"  ✗ Server library missing\"
fi
echo \"\"
echo \"Getting started:\"
echo \"  kolosal --help          - Show help\"
echo \"  kolosal-server --help   - Show server help\"
echo \"\"
echo \"Directory permissions:\"
echo \"  /var/lib/kolosal: \$(ls -ld /var/lib/kolosal 2>/dev/null | awk '{print \$1, \$3, \$4}' || echo 'not found')\"
echo \"  /var/lib/kolosal/models: \$(ls -ld /var/lib/kolosal/models 2>/dev/null | awk '{print \$1, \$3, \$4}' || echo 'not found')\"
echo \"  Current user groups: \$(groups)\"
echo \"\"
echo \"To start the server:\"
echo \"  kolosal-server --config /etc/kolosal/config.yaml\"
echo \"\"
echo \"Debugging information:\"
echo \"  Library path: /usr/lib\"
echo \"  Config path: /etc/kolosal\"
echo \"  Data path: /var/lib/kolosal\"
echo \"  Binary path: /usr/bin\"
echo \"\"
echo \"Testing server config detection...\"
if [ -f /usr/bin/kolosal-server ]; then
    timeout 3 /usr/bin/kolosal-server --port 8081 > /tmp/kolosal-config-test.log 2>&1 &
    CONFIG_TEST_PID=\$!
    sleep 1
    if kill -0 \$CONFIG_TEST_PID 2>/dev/null; then
        kill \$CONFIG_TEST_PID 2>/dev/null
        wait \$CONFIG_TEST_PID 2>/dev/null
        if grep -q \"/etc/kolosal/config.yaml\" /tmp/kolosal-config-test.log; then
            echo \"  ✓ Server correctly loads /etc/kolosal/config.yaml\"
        elif grep -q \"Loaded configuration from\" /tmp/kolosal-config-test.log; then
            echo \"  ⚠ Server loaded config from: \$(grep 'Loaded configuration from' /tmp/kolosal-config-test.log | head -1)\"
        else
            echo \"  ℹ Server started with default configuration\"
        fi
        rm -f /tmp/kolosal-config-test.log
    else
        echo \"  ⚠ Could not test server config detection\"
    fi
else
    echo \"  ✗ kolosal-server not found for testing\"
fi
echo \"\"
echo \"Checking primary config location:\"
if [ -f /etc/kolosal/config.yaml ]; then
    echo \"  ✓ Config found at /etc/kolosal/config.yaml\"
    echo \"  Config file size: \$(stat -c%s /etc/kolosal/config.yaml 2>/dev/null || echo 'unknown') bytes\"
else
    echo \"  ✗ Config missing at /etc/kolosal/config.yaml\"
fi
echo \"\"
echo \"Important notes:\"
echo \"  - The CLI will start kolosal-server in /var/lib/kolosal directory\"
echo \"  - This allows the server to create models/ subdirectory\"
echo \"  - If permission issues occur, check directory ownership and groups\"
echo \"  - Run 'groups' to see if you're in the kolosal group (logout/login may be required)\"
echo \"\"
echo \"If you encounter issues, check:\"
echo \"  1. Library dependencies: ldd /usr/bin/kolosal-server\"
echo \"  2. Library dependencies: ldd /usr/bin/kolosal\"
echo \"  3. Config file permissions: ls -la /etc/kolosal/config.yaml\"
echo \"  4. Test server config loading: kolosal-server --config /etc/kolosal/config.yaml --help\"
echo \"  5. Test CLI directly: kolosal --help\"
echo \"\"
echo \"To verify which config the server automatically detects:\"
echo \"  kolosal-server --port 8081 &\"
echo \"  # Check server startup messages - it will show which config file it loaded\"
echo \"  sleep 2 && pkill kolosal-server\"
echo \"  cat /tmp/kolosal-server.log\"
echo \"\"
echo \"Common solutions:\"
echo \"  - If libraries are missing: sudo apt update && sudo apt install --fix-broken\"
echo \"  - If config not found: sudo mkdir -p /etc/kolosal && sudo cp config.yaml /etc/kolosal/\"
echo \"  - Run: sudo ldconfig   # to refresh library cache\"
echo \"\"
if command -v systemctl >/dev/null 2>&1 && [ -f /etc/systemd/system/kolosal-server.service ]; then
echo \"Or use systemd service:\"
echo \"  sudo systemctl enable kolosal-server\"
echo \"  sudo systemctl start kolosal-server\"
echo \"\"
fi
echo \"For more information, visit:\"
echo \"  https://github.com/kolosalai/kolosal-cli\"
echo \"===============================================\"

exit 0
")
    configure_file(${POSTINST_SCRIPT_IN} ${POSTINST_SCRIPT} @ONLY)
    
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${POSTINST_SCRIPT}")
    
    # Pre-removal script for cleanup
    set(PRERM_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/prerm")
    file(WRITE ${PRERM_SCRIPT}
"#!/bin/bash
# Pre-removal script for Kolosal CLI

# Stop kolosal-server service if it's running
if command -v systemctl >/dev/null 2>&1; then
    if systemctl is-active --quiet kolosal-server; then
        echo \"Stopping kolosal-server service...\"
        systemctl stop kolosal-server
    fi
    
    if systemctl is-enabled --quiet kolosal-server 2>/dev/null; then
        echo \"Disabling kolosal-server service...\"
        systemctl disable kolosal-server
    fi
fi

exit 0
")
    
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${POSTINST_SCRIPT};${PRERM_SCRIPT}")
    
    # Post-removal script for final cleanup
    set(POSTRM_SCRIPT_IN "${CMAKE_CURRENT_BINARY_DIR}/postrm.in")
    set(POSTRM_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/postrm")
    file(WRITE ${POSTRM_SCRIPT_IN}
"#!/bin/bash
# Post-removal script for Kolosal CLI

# Reload systemd daemon if systemctl is available
if command -v systemctl >/dev/null 2>&1; then
    systemctl daemon-reload
fi

# Update library cache
if command -v ldconfig >/dev/null 2>&1; then
    ldconfig
fi

# Remove symbolic links we created
rm -f /usr/lib/x86_64-linux-gnu/libkolosal_server.so 2>/dev/null || true
rm -f \"/usr/lib/x86_64-linux-gnu/lib@INFERENCE_TARGET_NAME@.so\" 2>/dev/null || true

# Note: We preserve the kolosal user and /var/lib/kolosal for user data safety
echo \"Kolosal CLI has been removed.\"
echo \"User data in /var/lib/kolosal (if any) has been preserved.\"
echo \"The kolosal system user has been preserved for safety.\"
echo \"\"
echo \"To completely remove all data and user:\"
echo \"  sudo rm -rf /var/lib/kolosal\"
echo \"  sudo rm -rf /etc/kolosal\"
echo \"  sudo userdel kolosal\"

exit 0
")
    configure_file(${POSTRM_SCRIPT_IN} ${POSTRM_SCRIPT} @ONLY)
    
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${POSTINST_SCRIPT};${PRERM_SCRIPT};${POSTRM_SCRIPT}")
    
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
    
    # Base RPM dependencies
    set(CPACK_RPM_PACKAGE_REQUIRES "glibc >= 2.17, libgcc, libstdc++")
    
    # Add SSL/TLS support
    if(OpenSSL_FOUND)
        set(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, openssl-libs")
    endif()
    
    # Add CURL support
    if(CURL_FOUND)
        set(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, libcurl")
    endif()
    
    # Add compression support
    if(ZLIB_FOUND)
        set(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, zlib")
    endif()
    
    # Initialize RPM suggests list
    set(RPM_SUGGESTS_LIST "")
    
    # Add acceleration-specific dependencies
    if(USE_CUDA)
        set(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, cuda-runtime")
        set(RPM_SUGGESTS_LIST "cuda-devel")
    elseif(USE_VULKAN)
        set(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, vulkan-loader")
        set(RPM_SUGGESTS_LIST "vulkan-tools, vulkan-validation-layers")
    endif()
    
    # Set RPM suggests only if there are acceleration-specific suggestions
    if(RPM_SUGGESTS_LIST)
        set(CPACK_RPM_PACKAGE_SUGGESTS "${RPM_SUGGESTS_LIST}")
    endif()
    
    # RPM post-install and pre-uninstall scripts
    set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${POSTINST_SCRIPT}")
    set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${PRERM_SCRIPT}")
    set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${POSTRM_SCRIPT}")
    
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
    message(STATUS "  Package includes:")
    message(STATUS "    - kolosal.exe (main CLI)")
    message(STATUS "    - kolosal-server.exe (server executable)")
    message(STATUS "    - kolosal_server.dll (server library)")
    message(STATUS "    - ${INFERENCE_TARGET_NAME}.dll (inference engine)")
    message(STATUS "    - libcurl.dll and other dependencies")
    message(STATUS "    - config.yaml (configuration file)")
    message(STATUS "")
    message(STATUS "  Run 'cmake --build . --target package' to create the Windows installer")
    message(STATUS "  The installer will install to user profile and add to user PATH")
    message(STATUS "  After installation, both 'kolosal' and 'kolosal-server' will be available in PATH")
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
    message(STATUS "")
    message(STATUS "  Package includes:")
    message(STATUS "    - /usr/bin/kolosal (main CLI)")
    message(STATUS "    - /usr/bin/kolosal-server (server executable)")
    message(STATUS "    - /usr/lib/libkolosal_server.so (server library)")
    message(STATUS "    - /usr/lib/lib${INFERENCE_TARGET_NAME}.so (inference engine)")
    message(STATUS "    - /etc/kolosal/config.yaml (configuration)")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/kolosal-server/kolosal-server.service")
        message(STATUS "    - /etc/systemd/system/kolosal-server.service (systemd service)")
    endif()
    message(STATUS "    - Desktop entry and man page (if available)")
    message(STATUS "")
    message(STATUS "  Or use: make package")
    message(STATUS "")
    message(STATUS "Linux Build Requirements:")
    message(STATUS "  - libcurl4-openssl-dev (Ubuntu/Debian) or libcurl-devel (RHEL/CentOS)")
    message(STATUS "  - libssl-dev (Ubuntu/Debian) or openssl-devel (RHEL/CentOS)")
    message(STATUS "  - zlib1g-dev (Ubuntu/Debian) or zlib-devel (RHEL/CentOS)")
    message(STATUS "  - build-essential cmake (Ubuntu/Debian) or gcc-c++ cmake (RHEL/CentOS)")
    if(USE_VULKAN)
        message(STATUS "  - libvulkan-dev vulkan-tools (for Vulkan support)")
    elseif(USE_CUDA)
        message(STATUS "  - CUDA toolkit and runtime (for CUDA support)")
    endif()
endif()