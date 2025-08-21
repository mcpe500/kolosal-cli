cmake_minimum_required(VERSION 3.14)

project(kolosal-cli
  LANGUAGES C CXX
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

# macOS: broaden OS support and optionally build universal binaries
if(APPLE)
    # Option to build universal2 (arm64 + x86_64). Note: universal requires macOS 11.0+ for arm64 slices.
    option(BUILD_UNIVERSAL2 "Build universal (arm64;x86_64) binaries" OFF)
    if(BUILD_UNIVERSAL2)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Architectures for macOS build" FORCE)
    endif()

    # Decide minimum supported macOS version
    # If user didn't specify CMAKE_OSX_DEPLOYMENT_TARGET, choose a sensible default:
    #  - 11.0 when building for arm64 (incl. universal2)
    #  - 10.15 for Intel-only builds to support more Macs
    if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET OR CMAKE_OSX_DEPLOYMENT_TARGET STREQUAL "")
        set(__has_arm64 FALSE)
        if(CMAKE_OSX_ARCHITECTURES)
            string(FIND "${CMAKE_OSX_ARCHITECTURES}" "arm64" __arm64_index)
            if(NOT __arm64_index EQUAL -1)
                set(__has_arm64 TRUE)
            endif()
        endif()
        if(NOT __has_arm64 AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(__has_arm64 TRUE)
        endif()

        if(__has_arm64)
            set(MACOS_MIN_VERSION "11.0")
        else()
            set(MACOS_MIN_VERSION "10.15")
        endif()

        set(CMAKE_OSX_DEPLOYMENT_TARGET "${MACOS_MIN_VERSION}" CACHE STRING "macOS minimum deployment target" FORCE)
    else()
        # Respect user-provided target and use it for Info.plist too
        set(MACOS_MIN_VERSION "${CMAKE_OSX_DEPLOYMENT_TARGET}")
    endif()

    message(STATUS "macOS architectures: ${CMAKE_OSX_ARCHITECTURES}")
    message(STATUS "macOS deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")

    # Older macOS SDKs (< 13.3) mark std::to_chars(fp) unavailable; PoDoFo uses it.
    # Disable PoDoFo in that case to keep compatibility with older macOS versions.
    if(DEFINED CMAKE_OSX_DEPLOYMENT_TARGET AND CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS "13.3")
        set(USE_PODOFO OFF CACHE BOOL "Compile with PoDoFo PDF support" FORCE)
        message(STATUS "PoDoFo disabled for macOS < 13.3 to maximize compatibility")
    endif()
endif()

# Options
option(USE_CUDA "Enable CUDA support" OFF)
option(USE_VULKAN "Enable Vulkan support" OFF)
option(USE_MPI "Enable MPI support" OFF)
option(USE_METAL "Enable Apple Metal support" OFF)
option(ENABLE_NATIVE_OPTIMIZATION "Enable native CPU optimization (-march=native)" OFF)
option(ENABLE_CODESIGN "Enable code signing on macOS" OFF)
option(STRICT_CUDA_DEPENDS "If ON and building with CUDA, declare a hard DEB dependency on CUDA runtime; otherwise only recommend it" OFF)

# Code signing configuration for macOS
set(CODESIGN_IDENTITY "" CACHE STRING "Code signing identity (e.g., 'Developer ID Application: Your Name')")
set(CODESIGN_INSTALLER_IDENTITY "" CACHE STRING "Code signing identity for installer (e.g., 'Developer ID Installer: Your Name')")
set(DEVELOPER_TEAM_ID "" CACHE STRING "Apple Developer Team ID for notarization")

# Auto-enable Metal on Apple platforms
if(APPLE)
    set(USE_METAL ON CACHE BOOL "Enable Apple Metal support" FORCE)
    message(STATUS "Apple platform detected - enabling Metal support")
endif()

# Propagate options to subprojects
set(USE_CUDA ${USE_CUDA} CACHE BOOL "Enable CUDA support" FORCE)
set(USE_VULKAN ${USE_VULKAN} CACHE BOOL "Enable Vulkan support" FORCE)
set(USE_MPI ${USE_MPI} CACHE BOOL "Enable MPI support" FORCE)
set(USE_METAL ${USE_METAL} CACHE BOOL "Enable Apple Metal support" FORCE)
set(USE_PODOFO ${USE_PODOFO} CACHE BOOL "Compile with PoDoFo PDF support" FORCE)

# Add the kolosal-server submodule
add_subdirectory(kolosal-server)

# The inference engine is built as part of kolosal-server, but we keep this option for compatibility
option(BUILD_INFERENCE_ENGINE "Build the inference engine (built as part of kolosal-server)" ON)

# Determine the inference target name based on acceleration options
if(USE_CUDA)
    set(INFERENCE_TARGET_NAME "llama-cuda")
elseif(USE_VULKAN)
    set(INFERENCE_TARGET_NAME "llama-vulkan")
elseif(USE_METAL)
    set(INFERENCE_TARGET_NAME "llama-metal")
else()
    set(INFERENCE_TARGET_NAME "llama-cpu")
endif()

# Add external libraries
set(CURL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/curl)
set(NLOHMANN_JSON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)

# Include directories
if(WIN32)
    include_directories(${CURL_DIR}/include)
elseif(APPLE)
    # On macOS, use system CURL or Homebrew
    find_package(CURL REQUIRED)
    if(CURL_FOUND)
        include_directories(${CURL_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "CURL not found. Please install with: brew install curl")
    endif()
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
elseif(APPLE)
    # On macOS, CURL_LIBRARIES is already set by find_package
    set(CURL_LIBRARY ${CURL_LIBRARIES})
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

# Add macOS launcher executable (small C program that launches the shell script)
if(APPLE)
    add_executable(kolosal-launcher src/kolosal_launcher.c)
    set_target_properties(kolosal-launcher PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

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
elseif(APPLE)
    # macOS-specific libraries and frameworks
    target_link_libraries(kolosal pthread dl)
    
    # Link Apple frameworks for Metal support and system integration
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    target_link_libraries(kolosal ${FOUNDATION_FRAMEWORK} ${COREFOUNDATION_FRAMEWORK})
    
    # Add Metal frameworks if Metal support is enabled
    if(USE_METAL)
        find_library(METAL_FRAMEWORK Metal REQUIRED)
        find_library(METALKIT_FRAMEWORK MetalKit REQUIRED)
        target_link_libraries(kolosal ${METAL_FRAMEWORK} ${METALKIT_FRAMEWORK})
    endif()
    
    # Set output directory for macOS
    set_target_properties(kolosal PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
    
    # Link filesystem library for C++17 std::filesystem if needed
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
        target_link_libraries(kolosal stdc++fs)
    endif()
    
    # Find and link system libraries
    find_package(OpenSSL)
    if(OpenSSL_FOUND)
        target_link_libraries(kolosal OpenSSL::SSL OpenSSL::Crypto)
    endif()
    
    find_package(ZLIB)
    if(ZLIB_FOUND)
        target_link_libraries(kolosal ZLIB::ZLIB)
    endif()
    
    # Add macOS-specific compile definitions
    target_compile_definitions(kolosal PRIVATE 
        __APPLE__
        __MACOS__
    )
    
    # Apple Silicon vs Intel optimizations
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        # Apple Silicon (M1/M2/M3+) optimizations
        target_compile_definitions(kolosal PRIVATE __ARM64__)
        if(ENABLE_NATIVE_OPTIMIZATION)
            target_compile_options(kolosal PRIVATE -mcpu=apple-m1 -O3)
        endif()
    else()
        # Intel Mac optimizations
        target_compile_definitions(kolosal PRIVATE __X86_64__)
        if(ENABLE_NATIVE_OPTIMIZATION)
            target_compile_options(kolosal PRIVATE -march=native -mtune=native)
        endif()
    endif()
    
    # Apply optimization flags for Release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR NOT CMAKE_BUILD_TYPE)
        if(ENABLE_NATIVE_OPTIMIZATION)
            # Native optimizations already applied above
        else()
            target_compile_options(kolosal PRIVATE -O3)
        endif()
    endif()
    
    # Copy Kolosal server shared library (.dylib)
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:kolosal_server>"
        "${CMAKE_BINARY_DIR}/bin/"
        COMMENT "Copying kolosal_server library to output directory"
        DEPENDS kolosal_server
    )
    
    # Also copy to lib directory for standard library location
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/lib"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:kolosal_server>"
        "${CMAKE_BINARY_DIR}/lib/"
        COMMENT "Copying kolosal_server library to lib directory"
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
    
    # Copy the inference engine shared library (.dylib)
    if(BUILD_INFERENCE_ENGINE)
        add_custom_command(TARGET kolosal POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${INFERENCE_TARGET_NAME}>"
            "${CMAKE_BINARY_DIR}/bin/"
            COMMENT "Copying ${INFERENCE_TARGET_NAME} library to bin directory"
            DEPENDS ${INFERENCE_TARGET_NAME}
        )
        
        add_custom_command(TARGET kolosal POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/lib"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${INFERENCE_TARGET_NAME}>"
            "${CMAKE_BINARY_DIR}/lib/"
            COMMENT "Copying ${INFERENCE_TARGET_NAME} library to lib directory"
            DEPENDS ${INFERENCE_TARGET_NAME}
        )
        
        # Copy Metal-specific files if using Metal inference
        if(USE_METAL)
            # Add a post-build command to find and copy Metal files
            add_custom_command(TARGET kolosal POST_BUILD
                COMMAND ${CMAKE_COMMAND} 
                -DSOURCE_DIR="${CMAKE_BINARY_DIR}/kolosal-server"
                -DDEST_DIR="${CMAKE_BINARY_DIR}/bin"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyMetalFiles.cmake"
                COMMENT "Copying Metal GPU files to output directory"
                VERBATIM
            )
        endif()
    endif()
    
    # Set RPATH for runtime library search in the same directory and standard macOS paths
    set_target_properties(kolosal PROPERTIES
        INSTALL_RPATH "@executable_path;@executable_path/../lib;/usr/local/lib;/opt/homebrew/lib"
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
    
    # Set RPATH for kolosal-server executable as well
    set_target_properties(kolosal_server_exe PROPERTIES
        INSTALL_RPATH "@executable_path;@executable_path/../lib;/usr/local/lib;/opt/homebrew/lib"
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH_USE_LINK_PATH TRUE
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
    endif()
    
    # On Linux, check for required system libraries
    find_package(OpenSSL)
    if(OpenSSL_FOUND)
        target_link_libraries(kolosal OpenSSL::SSL OpenSSL::Crypto)
    endif()
    
    find_package(ZLIB)
    if(ZLIB_FOUND)
        target_link_libraries(kolosal ZLIB::ZLIB)
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
        else()
            target_compile_options(kolosal PRIVATE -O3)
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
    # Ensure the executable prefers our private runtime lib dir as well (order matters)
    set_target_properties(kolosal PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib/kolosal:$ORIGIN/../lib:$ORIGIN:/usr/lib/kolosal:/usr/lib"
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endif()

# Copy platform-specific config to output directory (Linux uses config.linux.yaml, macOS uses config.apple.yaml, Windows uses NSIS installer)
if(APPLE)
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.apple.yaml"
        "${CMAKE_BINARY_DIR}/bin/config.yaml"
        COMMENT "Copying config.apple.yaml to output directory as config.yaml"
    )
elseif(UNIX)
    # Minimal DEB packaging places all runtime artifacts under bin and excludes system-level files
    option(KOLOSAL_DEB_MINIMAL "If ON, Debian package includes only bin files and bundled runtime libs" ON)
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.linux.yaml"
        "${CMAKE_BINARY_DIR}/bin/config.yaml"
        COMMENT "Copying config.linux.yaml to output directory as config.yaml"
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
    
    # Install additional inference engine DLLs if they exist (regardless of build config)
    set(INFERENCE_DLL_CANDIDATES
        "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/llama-vulkan.dll"
        "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/llama-cuda.dll"
        "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/llama-cpu.dll"
        "${CMAKE_BINARY_DIR}/kolosal-server/$<CONFIG>/llama-vulkan.dll"
        "${CMAKE_BINARY_DIR}/kolosal-server/$<CONFIG>/llama-cuda.dll"
        "${CMAKE_BINARY_DIR}/kolosal-server/$<CONFIG>/llama-cpu.dll"
    )
    
    foreach(INFERENCE_DLL_PATH ${INFERENCE_DLL_CANDIDATES})
        install(FILES "${INFERENCE_DLL_PATH}"
            DESTINATION bin
            COMPONENT Runtime
            OPTIONAL
        )
    endforeach()
    
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
    
    # Install additional inference engine DLLs if they exist (fallback approach)
    install(CODE "
        # Define build output directories where inference DLLs might be found
        set(INFERENCE_OUTPUT_DIRS
            \"${CMAKE_BINARY_DIR}/bin/Debug\"
            \"${CMAKE_BINARY_DIR}/bin/Release\"
            \"${CMAKE_BINARY_DIR}/kolosal-server/Debug\"
            \"${CMAKE_BINARY_DIR}/kolosal-server/Release\"
            \"${CMAKE_BINARY_DIR}/kolosal-server/inference/Debug\"
            \"${CMAKE_BINARY_DIR}/kolosal-server/inference/Release\"
        )
        
        # Inference engine DLLs that might be available
        set(INFERENCE_DLLS
            \"llama-vulkan.dll\"
            \"llama-cuda.dll\"
            \"llama-cpu.dll\"
        )
        
        # Copy inference DLLs if found
        foreach(dll_name \${INFERENCE_DLLS})
            foreach(search_dir \${INFERENCE_OUTPUT_DIRS})
                set(dll_path \"\${search_dir}/\${dll_name}\")
                if(EXISTS \"\${dll_path}\")
                    file(INSTALL \"\${dll_path}\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\")
                    message(STATUS \"Installed inference engine: \${dll_name}\")
                    break()
                endif()
            endforeach()
        endforeach()
    ")
    
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
                    file(INSTALL \"\${dll_path}\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\")
                endif()
            endforeach()
        endif()
    ")
    
    # Install configuration file to the installation directory only (NSIS handles user config)
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
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
    
elseif(APPLE)
    # Install kolosal executable directly
    install(TARGETS kolosal
        RUNTIME DESTINATION "Kolosal.app/Contents/MacOS"
        COMPONENT Runtime
    )
    
    # Install the launcher executable that handles PATH setup
    install(TARGETS kolosal-launcher
        RUNTIME DESTINATION "Kolosal.app/Contents/MacOS"
        COMPONENT Runtime
    )
    
    # Install the launcher script that the C launcher will execute
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/kolosal-launcher.sh"
        DESTINATION "Kolosal.app/Contents/MacOS"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT Runtime
    )
    
    # Install kolosal-server executable
    install(TARGETS kolosal_server_exe
        RUNTIME DESTINATION "Kolosal.app/Contents/MacOS"
        COMPONENT Runtime
        OPTIONAL
    )
    
    # Fallback: Install kolosal-server executable by file path if target install fails
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/kolosal-server"
        DESTINATION "Kolosal.app/Contents/MacOS"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT Runtime
        OPTIONAL
    )
    
    # Configure app bundle Info.plist with correct minimum macOS version
    set(APP_PLIST_IN  "${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist.in")
    set(APP_PLIST_OUT "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
    configure_file(${APP_PLIST_IN} ${APP_PLIST_OUT} @ONLY)
    install(FILES "${APP_PLIST_OUT}"
        DESTINATION "Kolosal.app/Contents"
        RENAME Info.plist
        COMPONENT Runtime
    )
    
    # Install app bundle icon
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/kolosal.icns"
        DESTINATION "Kolosal.app/Contents/Resources"
        COMPONENT Runtime
    )
    
    # Install shared libraries to Frameworks directory (standard app bundle structure)
    # Try to install the kolosal_server target if it exists
    install(TARGETS kolosal_server
        LIBRARY DESTINATION "Kolosal.app/Contents/Frameworks"
        ARCHIVE DESTINATION "Kolosal.app/Contents/Frameworks"
        COMPONENT Runtime
        OPTIONAL
    )
    
    # Manual installation of kolosal_server library using install(CODE)
    install(CODE "
        set(LIB_SRC \"${CMAKE_CURRENT_BINARY_DIR}/lib/libkolosal_server.dylib\")
        set(LIB_DST \"\${CMAKE_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/libkolosal_server.dylib\")
        if(EXISTS \"\${LIB_SRC}\")
            message(STATUS \"Installing kolosal_server library: \${LIB_SRC} -> \${LIB_DST}\")
            file(COPY \"\${LIB_SRC}\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks\")
            execute_process(COMMAND chmod +x \"\${LIB_DST}\")
        else()
            message(WARNING \"kolosal_server library not found at: \${LIB_SRC}\")
        endif()
    " COMPONENT Runtime)
    
    # Install the inference engine shared library
    if(BUILD_INFERENCE_ENGINE)
        install(TARGETS ${INFERENCE_TARGET_NAME}
            LIBRARY DESTINATION "Kolosal.app/Contents/Frameworks"
            ARCHIVE DESTINATION "Kolosal.app/Contents/Frameworks"
            COMPONENT Runtime
        )
    endif()
    
    # Install additional inference engine shared libraries if they exist (macOS .dylib files)
    set(INFERENCE_DYLIB_CANDIDATES
        "${CMAKE_BINARY_DIR}/lib/libllama-metal.dylib"
        "${CMAKE_BINARY_DIR}/lib/libllama-vulkan.dylib"
        "${CMAKE_BINARY_DIR}/lib/libllama-cuda.dylib"
        "${CMAKE_BINARY_DIR}/lib/libllama-cpu.dylib"
        "${CMAKE_BINARY_DIR}/bin/libllama-metal.dylib"
        "${CMAKE_BINARY_DIR}/bin/libllama-vulkan.dylib"
        "${CMAKE_BINARY_DIR}/bin/libllama-cuda.dylib"
        "${CMAKE_BINARY_DIR}/bin/libllama-cpu.dylib"
        "${CMAKE_BINARY_DIR}/kolosal-server/lib/libllama-metal.dylib"
        "${CMAKE_BINARY_DIR}/kolosal-server/lib/libllama-vulkan.dylib"
        "${CMAKE_BINARY_DIR}/kolosal-server/lib/libllama-cuda.dylib"
        "${CMAKE_BINARY_DIR}/kolosal-server/lib/libllama-cpu.dylib"
        "${CMAKE_BINARY_DIR}/kolosal-server/build/lib/libllama-metal.dylib"
        "${CMAKE_BINARY_DIR}/kolosal-server/build/lib/libllama-vulkan.dylib"
        "${CMAKE_BINARY_DIR}/kolosal-server/build/lib/libllama-cuda.dylib"
        "${CMAKE_BINARY_DIR}/kolosal-server/build/lib/libllama-cpu.dylib"
    )
    
    foreach(INFERENCE_DYLIB_PATH ${INFERENCE_DYLIB_CANDIDATES})
        install(FILES "${INFERENCE_DYLIB_PATH}"
            DESTINATION "Kolosal.app/Contents/Frameworks"
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
            COMPONENT Runtime
            OPTIONAL
        )
    endforeach()
    
    # Install external libraries from Homebrew that are required dependencies
    # These libraries are dynamically linked and need to be bundled with the app
    set(EXTERNAL_DYLIB_CANDIDATES
        # OpenSSL libraries
        "/opt/homebrew/opt/openssl@3/lib/libssl.3.dylib"
        "/opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib"
        "/opt/homebrew/lib/libssl.3.dylib"
        "/opt/homebrew/lib/libcrypto.3.dylib"
        "/usr/local/opt/openssl@3/lib/libssl.3.dylib"
        "/usr/local/opt/openssl@3/lib/libcrypto.3.dylib"
        "/usr/local/lib/libssl.3.dylib"
        "/usr/local/lib/libcrypto.3.dylib"
        
        # FontConfig library
        "/opt/homebrew/opt/fontconfig/lib/libfontconfig.1.dylib"
        "/opt/homebrew/lib/libfontconfig.1.dylib"
        "/usr/local/opt/fontconfig/lib/libfontconfig.1.dylib"
        "/usr/local/lib/libfontconfig.1.dylib"
        
        # FreeType library
        "/opt/homebrew/opt/freetype/lib/libfreetype.6.dylib"
        "/opt/homebrew/lib/libfreetype.6.dylib"
        "/usr/local/opt/freetype/lib/libfreetype.6.dylib"
        "/usr/local/lib/libfreetype.6.dylib"
        
        # LibPNG library
        "/opt/homebrew/opt/libpng/lib/libpng16.16.dylib"
        "/opt/homebrew/lib/libpng16.16.dylib"
        "/usr/local/opt/libpng/lib/libpng16.16.dylib"
        "/usr/local/lib/libpng16.16.dylib"
        
        # LibTIFF library and its dependencies
        "/opt/homebrew/opt/libtiff/lib/libtiff.6.dylib"
        "/opt/homebrew/lib/libtiff.6.dylib"
        "/usr/local/opt/libtiff/lib/libtiff.6.dylib"
        "/usr/local/lib/libtiff.6.dylib"
        "/opt/homebrew/opt/zstd/lib/libzstd.1.dylib"
        "/opt/homebrew/lib/libzstd.1.dylib"
        "/usr/local/opt/zstd/lib/libzstd.1.dylib"
        "/usr/local/lib/libzstd.1.dylib"
        "/opt/homebrew/opt/xz/lib/liblzma.5.dylib"
        "/opt/homebrew/lib/liblzma.5.dylib"
        "/usr/local/opt/xz/lib/liblzma.5.dylib"
        "/usr/local/lib/liblzma.5.dylib"
        
        # JPEG library
        "/opt/homebrew/opt/jpeg-turbo/lib/libjpeg.8.dylib"
        "/opt/homebrew/lib/libjpeg.8.dylib"
        "/usr/local/opt/jpeg-turbo/lib/libjpeg.8.dylib"
        "/usr/local/lib/libjpeg.8.dylib"
        "/opt/homebrew/opt/jpeg/lib/libjpeg.9.dylib"
        "/opt/homebrew/lib/libjpeg.9.dylib"
        "/usr/local/opt/jpeg/lib/libjpeg.9.dylib"
        "/usr/local/lib/libjpeg.9.dylib"
    )
    
    foreach(EXTERNAL_DYLIB_PATH ${EXTERNAL_DYLIB_CANDIDATES})
        install(CODE "
            if(EXISTS \"${EXTERNAL_DYLIB_PATH}\")
                # Resolve symlinks to get the actual file
                get_filename_component(RESOLVED_PATH \"${EXTERNAL_DYLIB_PATH}\" REALPATH)
                get_filename_component(LIB_NAME \"${EXTERNAL_DYLIB_PATH}\" NAME)
                
                # Copy the actual file (not symlink) to the destination
                file(COPY \"\${RESOLVED_PATH}\"
                     DESTINATION \"\${CMAKE_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks\"
                     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
                
                # Rename the copied file to the expected name (in case the resolved name is different)
                get_filename_component(RESOLVED_NAME \"\${RESOLVED_PATH}\" NAME)
                if(NOT \"\${RESOLVED_NAME}\" STREQUAL \"\${LIB_NAME}\")
                    file(RENAME 
                        \"\${CMAKE_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/\${RESOLVED_NAME}\"
                        \"\${CMAKE_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/\${LIB_NAME}\")
                endif()
                
                message(STATUS \"Copied external library: \${LIB_NAME} (resolved from \${RESOLVED_PATH})\")
            endif()
        " COMPONENT Runtime)
    endforeach()
    
    # Install Metal-specific files if using Metal inference
    if(USE_METAL)
        # Install Metal files to Resources directory
        install(FILES 
            "${CMAKE_BINARY_DIR}/bin/ggml-common.h"
            "${CMAKE_BINARY_DIR}/bin/ggml-metal-impl.h"
            "${CMAKE_BINARY_DIR}/bin/ggml-metal.metal"
            "${CMAKE_BINARY_DIR}/kolosal-server/build/bin/ggml-common.h"
            "${CMAKE_BINARY_DIR}/kolosal-server/build/bin/ggml-metal-impl.h"
            "${CMAKE_BINARY_DIR}/kolosal-server/build/bin/ggml-metal.metal"
            DESTINATION "Kolosal.app/Contents/Resources"
            PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
            COMPONENT Runtime
            OPTIONAL
        )
    endif()
    
    # Install configuration file to Resources directory
    install(FILES "${CMAKE_BINARY_DIR}/bin/config.yaml"
        DESTINATION "Kolosal.app/Contents/Resources"
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
        COMPONENT Runtime
    )
    
    # Install README and LICENSE to Resources
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
            DESTINATION "Kolosal.app/Contents/Resources"
            COMPONENT Runtime
        )
    endif()
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
            DESTINATION "Kolosal.app/Contents/Resources"
            COMPONENT Runtime
        )
    endif()
    
    # Install post-install script to fix RPATH and bundle dependencies
    install(CODE "
        # Determine the actual install destination (handles DESTDIR)
        if(DEFINED ENV{DESTDIR})
            set(ACTUAL_INSTALL_PREFIX \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}\")
        else()
            set(ACTUAL_INSTALL_PREFIX \"\${CMAKE_INSTALL_PREFIX}\")
        endif()
        
        # Fix RPATH for executables to find libraries in relative Frameworks directory
        if(EXISTS \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/MacOS/kolosal\")
            execute_process(COMMAND install_name_tool -add_rpath @executable_path/../Frameworks \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/MacOS/kolosal\")
        endif()
        if(EXISTS \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/MacOS/kolosal-server\")
            execute_process(COMMAND install_name_tool -add_rpath @executable_path/../Frameworks \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/MacOS/kolosal-server\")
        endif()
        
        # Fix RPATH for the kolosal_server library
        if(EXISTS \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/libkolosal_server.dylib\")
            execute_process(COMMAND install_name_tool -id @rpath/libkolosal_server.dylib \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/libkolosal_server.dylib\")
        endif()
        
        # Fix RPATH for inference libraries
        file(GLOB INFERENCE_LIBS \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/libllama-*.dylib\")
        foreach(INFERENCE_LIB \${INFERENCE_LIBS})
            get_filename_component(LIB_NAME \"\${INFERENCE_LIB}\" NAME)
            execute_process(COMMAND install_name_tool -id @rpath/\${LIB_NAME} \"\${INFERENCE_LIB}\")
        endforeach()
        
        # Fix RPATH and dependencies for external libraries
        set(EXTERNAL_LIBS_TO_FIX
            \"libssl.3.dylib\"
            \"libcrypto.3.dylib\"
            \"libfontconfig.1.dylib\"
            \"libfreetype.6.dylib\"
            \"libpng16.16.dylib\"
            \"libtiff.6.dylib\"
            \"libjpeg.8.dylib\"
            \"libjpeg.9.dylib\"
            \"libzstd.1.dylib\"
            \"liblzma.5.dylib\"
        )
        
        foreach(LIB_NAME \${EXTERNAL_LIBS_TO_FIX})
            set(LIB_PATH \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/\${LIB_NAME}\")
            if(EXISTS \"\${LIB_PATH}\")
                # Fix the library's install name to use @rpath
                execute_process(COMMAND install_name_tool -id @rpath/\${LIB_NAME} \"\${LIB_PATH}\")
                
                # Fix dependencies pointing to Homebrew paths
                execute_process(COMMAND otool -L \"\${LIB_PATH}\" OUTPUT_VARIABLE OTOOL_OUTPUT)
                
                # Update OpenSSL dependencies - handle both opt and Cellar paths
                string(REGEX MATCHALL \"/[^\\t\\n ]+/libssl\\.3\\.dylib\" SSL_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(SSL_DEP \${SSL_DEPS})
                    if(SSL_DEP MATCHES \"/(opt|Cellar)/.*openssl.*\")
                        execute_process(COMMAND install_name_tool -change \"\${SSL_DEP}\" @rpath/libssl.3.dylib \"\${LIB_PATH}\")
                        message(STATUS \"Fixed SSL dependency: \${SSL_DEP} -> @rpath/libssl.3.dylib\")
                    endif()
                endforeach()
                
                string(REGEX MATCHALL \"/[^\\t\\n ]+/libcrypto\\.3\\.dylib\" CRYPTO_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(CRYPTO_DEP \${CRYPTO_DEPS})
                    if(CRYPTO_DEP MATCHES \"/(opt|Cellar)/.*openssl.*\")
                        execute_process(COMMAND install_name_tool -change \"\${CRYPTO_DEP}\" @rpath/libcrypto.3.dylib \"\${LIB_PATH}\")
                        message(STATUS \"Fixed Crypto dependency: \${CRYPTO_DEP} -> @rpath/libcrypto.3.dylib\")
                    endif()
                endforeach()
                
                # Update FreeType dependencies - handle both opt and Cellar paths
                string(REGEX MATCHALL \"/[^\\t\\n ]+/libfreetype\\.6\\.dylib\" FREETYPE_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(FREETYPE_DEP \${FREETYPE_DEPS})
                    if(FREETYPE_DEP MATCHES \"/(opt|Cellar)/.*freetype.*\")
                        execute_process(COMMAND install_name_tool -change \"\${FREETYPE_DEP}\" @rpath/libfreetype.6.dylib \"\${LIB_PATH}\")
                        message(STATUS \"Fixed FreeType dependency: \${FREETYPE_DEP} -> @rpath/libfreetype.6.dylib\")
                    endif()
                endforeach()
                
                # Update LibPNG dependencies - handle both opt and Cellar paths
                string(REGEX MATCHALL \"/[^\\t\\n ]+/libpng16\\.16\\.dylib\" PNG_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(PNG_DEP \${PNG_DEPS})
                    if(PNG_DEP MATCHES \"/(opt|Cellar)/.*libpng.*\")
                        execute_process(COMMAND install_name_tool -change \"\${PNG_DEP}\" @rpath/libpng16.16.dylib \"\${LIB_PATH}\")
                        message(STATUS \"Fixed PNG dependency: \${PNG_DEP} -> @rpath/libpng16.16.dylib\")
                    endif()
                endforeach()
                
                # Update LibTIFF dependencies - handle both opt and Cellar paths
                string(REGEX MATCHALL \"/[^\\t\\n ]+/libtiff\\.6\\.dylib\" TIFF_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(TIFF_DEP \${TIFF_DEPS})
                    if(TIFF_DEP MATCHES \"/(opt|Cellar)/.*libtiff.*\")
                        execute_process(COMMAND install_name_tool -change \"\${TIFF_DEP}\" @rpath/libtiff.6.dylib \"\${LIB_PATH}\")
                        message(STATUS \"Fixed TIFF dependency: \${TIFF_DEP} -> @rpath/libtiff.6.dylib\")
                    endif()
                endforeach()
                
                # Update JPEG dependencies - handle both opt and Cellar paths
                string(REGEX MATCHALL \"/[^\\t\\n ]+/libjpeg\\.[0-9]+\\.dylib\" JPEG_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(JPEG_DEP \${JPEG_DEPS})
                    if(JPEG_DEP MATCHES \"/(opt|Cellar)/.*(jpeg|jpeg-turbo).*\")
                        get_filename_component(JPEG_NAME \"\${JPEG_DEP}\" NAME)
                        execute_process(COMMAND install_name_tool -change \"\${JPEG_DEP}\" @rpath/\${JPEG_NAME} \"\${LIB_PATH}\")
                        message(STATUS \"Fixed JPEG dependency: \${JPEG_DEP} -> @rpath/\${JPEG_NAME}\")
                    endif()
                endforeach()
                
                # Update FontConfig dependencies - handle both opt and Cellar paths
                string(REGEX MATCHALL \"/[^\\t\\n ]+/libfontconfig\\.1\\.dylib\" FONTCONFIG_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(FONTCONFIG_DEP \${FONTCONFIG_DEPS})
                    if(FONTCONFIG_DEP MATCHES \"/(opt|Cellar)/.*fontconfig.*\")
                        execute_process(COMMAND install_name_tool -change \"\${FONTCONFIG_DEP}\" @rpath/libfontconfig.1.dylib \"\${LIB_PATH}\")
                        message(STATUS \"Fixed FontConfig dependency: \${FONTCONFIG_DEP} -> @rpath/libfontconfig.1.dylib\")
                    endif()
                endforeach()
                
                # Update zstd dependencies (for LibTIFF) - handle both opt and Cellar paths
                string(REGEX MATCHALL \"/[^\\t\\n ]+/libzstd\\.1\\.dylib\" ZSTD_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(ZSTD_DEP \${ZSTD_DEPS})
                    if(ZSTD_DEP MATCHES \"/(opt|Cellar)/.*zstd.*\")
                        execute_process(COMMAND install_name_tool -change \"\${ZSTD_DEP}\" @rpath/libzstd.1.dylib \"\${LIB_PATH}\")
                        message(STATUS \"Fixed zstd dependency: \${ZSTD_DEP} -> @rpath/libzstd.1.dylib\")
                    endif()
                endforeach()
                
                # Update liblzma/xz dependencies (for LibTIFF) - handle both opt and Cellar paths
                string(REGEX MATCHALL \"/[^\\t\\n ]+/liblzma\\.5\\.dylib\" LZMA_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(LZMA_DEP \${LZMA_DEPS})
                    if(LZMA_DEP MATCHES \"/(opt|Cellar)/.*xz.*\")
                        execute_process(COMMAND install_name_tool -change \"\${LZMA_DEP}\" @rpath/liblzma.5.dylib \"\${LIB_PATH}\")
                        message(STATUS \"Fixed lzma dependency: \${LZMA_DEP} -> @rpath/liblzma.5.dylib\")
                    endif()
                endforeach()
                
                message(STATUS \"Fixed RPATH and dependencies for \${LIB_NAME}\")
            endif()
        endforeach()
        
        # Fix main executables and libraries to use @rpath for external dependencies
        set(MAIN_BINARIES_TO_FIX
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/MacOS/kolosal\"
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/MacOS/kolosal-server\"
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/libkolosal_server.dylib\"
        )
        
        # Also fix inference libraries
        file(GLOB INFERENCE_LIBS_TO_FIX \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/libllama-*.dylib\")
        list(APPEND MAIN_BINARIES_TO_FIX \${INFERENCE_LIBS_TO_FIX})
        
        foreach(BINARY_PATH \${MAIN_BINARIES_TO_FIX})
            if(EXISTS \"\${BINARY_PATH}\")
                execute_process(COMMAND otool -L \"\${BINARY_PATH}\" OUTPUT_VARIABLE OTOOL_OUTPUT)
                
                # Update OpenSSL dependencies - handle both opt and Cellar paths
                string(REGEX MATCHALL \"/[^\\t\\n ]+/libssl\\.3\\.dylib\" SSL_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(SSL_DEP \${SSL_DEPS})
                    if(SSL_DEP MATCHES \"/(opt|Cellar)/.*openssl.*\")
                        execute_process(COMMAND install_name_tool -change \"\${SSL_DEP}\" @rpath/libssl.3.dylib \"\${BINARY_PATH}\")
                        message(STATUS \"Fixed SSL dependency in \${BINARY_PATH}: \${SSL_DEP} -> @rpath/libssl.3.dylib\")
                    endif()
                endforeach()
                
                string(REGEX MATCHALL \"/[^\\t\\n ]+/libcrypto\\.3\\.dylib\" CRYPTO_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(CRYPTO_DEP \${CRYPTO_DEPS})
                    if(CRYPTO_DEP MATCHES \"/(opt|Cellar)/.*openssl.*\")
                        execute_process(COMMAND install_name_tool -change \"\${CRYPTO_DEP}\" @rpath/libcrypto.3.dylib \"\${BINARY_PATH}\")
                        message(STATUS \"Fixed Crypto dependency in \${BINARY_PATH}: \${CRYPTO_DEP} -> @rpath/libcrypto.3.dylib\")
                    endif()
                endforeach()
                
                # Update FreeType dependencies - handle both opt and Cellar paths
                string(REGEX MATCHALL \"/[^\\t\\n ]+/libfreetype\\.6\\.dylib\" FREETYPE_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(FREETYPE_DEP \${FREETYPE_DEPS})
                    if(FREETYPE_DEP MATCHES \"/(opt|Cellar)/.*freetype.*\")
                        execute_process(COMMAND install_name_tool -change \"\${FREETYPE_DEP}\" @rpath/libfreetype.6.dylib \"\${BINARY_PATH}\")
                        message(STATUS \"Fixed FreeType dependency in \${BINARY_PATH}: \${FREETYPE_DEP} -> @rpath/libfreetype.6.dylib\")
                    endif()
                endforeach()
                
                # Update LibPNG dependencies - handle both opt and Cellar paths
                string(REGEX MATCHALL \"/[^\\t\\n ]+/libpng16\\.16\\.dylib\" PNG_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(PNG_DEP \${PNG_DEPS})
                    if(PNG_DEP MATCHES \"/(opt|Cellar)/.*libpng.*\")
                        execute_process(COMMAND install_name_tool -change \"\${PNG_DEP}\" @rpath/libpng16.16.dylib \"\${BINARY_PATH}\")
                        message(STATUS \"Fixed PNG dependency in \${BINARY_PATH}: \${PNG_DEP} -> @rpath/libpng16.16.dylib\")
                    endif()
                endforeach()
                
                # Update LibTIFF dependencies - handle both opt and Cellar paths
                string(REGEX MATCHALL \"/[^\\t\\n ]+/libtiff\\.6\\.dylib\" TIFF_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(TIFF_DEP \${TIFF_DEPS})
                    if(TIFF_DEP MATCHES \"/(opt|Cellar)/.*libtiff.*\")
                        execute_process(COMMAND install_name_tool -change \"\${TIFF_DEP}\" @rpath/libtiff.6.dylib \"\${BINARY_PATH}\")
                        message(STATUS \"Fixed TIFF dependency in \${BINARY_PATH}: \${TIFF_DEP} -> @rpath/libtiff.6.dylib\")
                    endif()
                endforeach()
                
                # Update JPEG dependencies - handle both opt and Cellar paths
                string(REGEX MATCHALL \"/[^\\t\\n ]+/libjpeg\\.[0-9]+\\.dylib\" JPEG_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(JPEG_DEP \${JPEG_DEPS})
                    if(JPEG_DEP MATCHES \"/(opt|Cellar)/.*(jpeg|jpeg-turbo).*\")
                        get_filename_component(JPEG_NAME \"\${JPEG_DEP}\" NAME)
                        execute_process(COMMAND install_name_tool -change \"\${JPEG_DEP}\" @rpath/\${JPEG_NAME} \"\${BINARY_PATH}\")
                        message(STATUS \"Fixed JPEG dependency in \${BINARY_PATH}: \${JPEG_DEP} -> @rpath/\${JPEG_NAME}\")
                    endif()
                endforeach()
                
                # Update FontConfig dependencies - handle both opt and Cellar paths
                string(REGEX MATCHALL \"/[^\\t\\n ]+/libfontconfig\\.1\\.dylib\" FONTCONFIG_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(FONTCONFIG_DEP \${FONTCONFIG_DEPS})
                    if(FONTCONFIG_DEP MATCHES \"/(opt|Cellar)/.*fontconfig.*\")
                        execute_process(COMMAND install_name_tool -change \"\${FONTCONFIG_DEP}\" @rpath/libfontconfig.1.dylib \"\${BINARY_PATH}\")
                        message(STATUS \"Fixed FontConfig dependency in \${BINARY_PATH}: \${FONTCONFIG_DEP} -> @rpath/libfontconfig.1.dylib\")
                    endif()
                endforeach()
                
                # Update zstd dependencies (for LibTIFF) - handle both opt and Cellar paths
                string(REGEX MATCHALL \"/[^\\t\\n ]+/libzstd\\.1\\.dylib\" ZSTD_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(ZSTD_DEP \${ZSTD_DEPS})
                    if(ZSTD_DEP MATCHES \"/(opt|Cellar)/.*zstd.*\")
                        execute_process(COMMAND install_name_tool -change \"\${ZSTD_DEP}\" @rpath/libzstd.1.dylib \"\${BINARY_PATH}\")
                        message(STATUS \"Fixed zstd dependency in \${BINARY_PATH}: \${ZSTD_DEP} -> @rpath/libzstd.1.dylib\")
                    endif()
                endforeach()
                
                # Update liblzma/xz dependencies (for LibTIFF) - handle both opt and Cellar paths
                string(REGEX MATCHALL \"/[^\\t\\n ]+/liblzma\\.5\\.dylib\" LZMA_DEPS \"\${OTOOL_OUTPUT}\")
                foreach(LZMA_DEP \${LZMA_DEPS})
                    if(LZMA_DEP MATCHES \"/(opt|Cellar)/.*xz.*\")
                        execute_process(COMMAND install_name_tool -change \"\${LZMA_DEP}\" @rpath/liblzma.5.dylib \"\${BINARY_PATH}\")
                        message(STATUS \"Fixed lzma dependency in \${BINARY_PATH}: \${LZMA_DEP} -> @rpath/liblzma.5.dylib\")
                    endif()
                endforeach()
                
                get_filename_component(BINARY_NAME \"\${BINARY_PATH}\" NAME)
                message(STATUS \"Fixed external dependencies for \${BINARY_NAME}\")
            endif()
        endforeach()
        
        message(STATUS \"Fixed RPATH for all installed libraries in \${ACTUAL_INSTALL_PREFIX}\")
        
        # Re-sign all external libraries and binaries with proper code signing
        message(STATUS \"Setting up code signing for macOS distribution...\")
        
        # Check if code signing is enabled and we have a signing identity
        set(SHOULD_CODESIGN OFF)
        if(\"${ENABLE_CODESIGN}\" AND NOT \"${CODESIGN_IDENTITY}\" STREQUAL \"\")
            set(SHOULD_CODESIGN ON)
            message(STATUS \"Code signing enabled with identity: ${CODESIGN_IDENTITY}\")
        else()
            message(STATUS \"Code signing disabled or no identity provided - using ad-hoc signing\")
        endif()
        
        set(LIBS_TO_SIGN
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/libssl.3.dylib\"
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/libcrypto.3.dylib\"
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/libfontconfig.1.dylib\"
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/libfreetype.6.dylib\"
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/libpng16.16.dylib\"
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/libtiff.6.dylib\"
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/libjpeg.8.dylib\"
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/libjpeg.9.dylib\"
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/libzstd.1.dylib\"
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/liblzma.5.dylib\"
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/libkolosal_server.dylib\"
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/Frameworks/libllama-metal.dylib\"
        )
        
        set(BINARIES_TO_SIGN
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/MacOS/kolosal-server\"
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/MacOS/kolosal\"
            \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/MacOS/kolosal-launcher\"
        )
        
        # Sign all libraries first
        foreach(LIB_TO_SIGN \${LIBS_TO_SIGN})
            if(EXISTS \"\${LIB_TO_SIGN}\")
                get_filename_component(LIB_NAME \"\${LIB_TO_SIGN}\" NAME)
                
                # Remove existing signature first
                execute_process(
                    COMMAND codesign --remove-signature \"\${LIB_TO_SIGN}\"
                    ERROR_QUIET
                )
                
                # Sign with appropriate identity
                if(\${SHOULD_CODESIGN})
                    execute_process(
                        COMMAND codesign --force --sign \"${CODESIGN_IDENTITY}\" 
                               --timestamp --options runtime \"\${LIB_TO_SIGN}\"
                        RESULT_VARIABLE SIGN_RESULT
                        ERROR_QUIET
                    )
                    if(SIGN_RESULT EQUAL 0)
                        message(STATUS \"Successfully signed library \${LIB_NAME} with Developer ID\")
                    else()
                        message(WARNING \"Failed to sign \${LIB_NAME} with Developer ID, falling back to ad-hoc\")
                        execute_process(
                            COMMAND codesign -s - --preserve-metadata=entitlements,requirements,flags,runtime \"\${LIB_TO_SIGN}\"
                            ERROR_QUIET
                        )
                    endif()
                else()
                    # Ad-hoc sign the library
                    execute_process(
                        COMMAND codesign -s - --preserve-metadata=entitlements,requirements,flags,runtime \"\${LIB_TO_SIGN}\"
                        RESULT_VARIABLE SIGN_RESULT
                        ERROR_QUIET
                    )
                    if(SIGN_RESULT EQUAL 0)
                        message(STATUS \"Successfully ad-hoc signed library \${LIB_NAME}\")
                    else()
                        message(WARNING \"Failed to sign \${LIB_NAME}\")
                    endif()
                endif()
            endif()
        endforeach()
        
        # Sign all binaries
        foreach(BINARY_TO_SIGN \${BINARIES_TO_SIGN})
            if(EXISTS \"\${BINARY_TO_SIGN}\")
                get_filename_component(BINARY_NAME \"\${BINARY_TO_SIGN}\" NAME)
                
                # Remove existing signature first
                execute_process(
                    COMMAND codesign --remove-signature \"\${BINARY_TO_SIGN}\"
                    ERROR_QUIET
                )
                
                # Sign with appropriate identity
                if(\${SHOULD_CODESIGN})
                    # Create entitlements for the main executables
                    if(\"\${BINARY_NAME}\" STREQUAL \"kolosal\" OR \"\${BINARY_NAME}\" STREQUAL \"kolosal-server\" OR \"\${BINARY_NAME}\" STREQUAL \"kolosal-launcher\")
                        set(ENTITLEMENTS_FILE \"\${CMAKE_CURRENT_SOURCE_DIR}/resources/kolosal.entitlements\")
                        if(EXISTS \"\${ENTITLEMENTS_FILE}\")
                            set(ENTITLEMENTS_ARG --entitlements \"\${ENTITLEMENTS_FILE}\")
                        else()
                            set(ENTITLEMENTS_ARG \"\")
                        endif()
                    else()
                        set(ENTITLEMENTS_ARG \"\")
                    endif()
                    
                    execute_process(
                        COMMAND codesign --force --sign \"${CODESIGN_IDENTITY}\" 
                               --timestamp --options runtime \${ENTITLEMENTS_ARG} \"\${BINARY_TO_SIGN}\"
                        RESULT_VARIABLE SIGN_RESULT
                        ERROR_QUIET
                    )
                    if(SIGN_RESULT EQUAL 0)
                        message(STATUS \"Successfully signed binary \${BINARY_NAME} with Developer ID\")
                    else()
                        message(WARNING \"Failed to sign \${BINARY_NAME} with Developer ID, falling back to ad-hoc\")
                        execute_process(
                            COMMAND codesign -s - --preserve-metadata=entitlements,requirements,flags,runtime \"\${BINARY_TO_SIGN}\"
                            ERROR_QUIET
                        )
                    endif()
                else()
                    # Ad-hoc sign the binary
                    execute_process(
                        COMMAND codesign -s - --preserve-metadata=entitlements,requirements,flags,runtime \"\${BINARY_TO_SIGN}\"
                        RESULT_VARIABLE SIGN_RESULT
                        ERROR_QUIET
                    )
                    if(SIGN_RESULT EQUAL 0)
                        message(STATUS \"Successfully ad-hoc signed binary \${BINARY_NAME}\")
                    else()
                        message(WARNING \"Failed to sign \${BINARY_NAME}\")
                    endif()
                endif()
            endif()
        endforeach()
        
        # Finally, sign the entire app bundle
        if(EXISTS \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app\")
            if(\${SHOULD_CODESIGN})
                set(ENTITLEMENTS_FILE \"\${CMAKE_CURRENT_SOURCE_DIR}/resources/kolosal.entitlements\")
                if(EXISTS \"\${ENTITLEMENTS_FILE}\")
                    set(ENTITLEMENTS_ARG --entitlements \"\${ENTITLEMENTS_FILE}\")
                else()
                    set(ENTITLEMENTS_ARG \"\")
                endif()
                
                execute_process(
                    COMMAND codesign --force --sign \"${CODESIGN_IDENTITY}\" 
                           --timestamp --options runtime \${ENTITLEMENTS_ARG} \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app\"
                    RESULT_VARIABLE SIGN_RESULT
                    ERROR_QUIET
                )
                if(SIGN_RESULT EQUAL 0)
                    message(STATUS \"Successfully signed Kolosal.app bundle with Developer ID\")
                else()
                    message(WARNING \"Failed to sign app bundle with Developer ID, falling back to ad-hoc\")
                    execute_process(
                        COMMAND codesign -s - --preserve-metadata=entitlements,requirements,flags,runtime \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app\"
                        ERROR_QUIET
                    )
                endif()
            else()
                # Ad-hoc sign the app bundle
                execute_process(
                    COMMAND codesign -s - --preserve-metadata=entitlements,requirements,flags,runtime \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app\"
                    RESULT_VARIABLE SIGN_RESULT
                    ERROR_QUIET
                )
                if(SIGN_RESULT EQUAL 0)
                    message(STATUS \"Successfully ad-hoc signed Kolosal.app bundle\")
                else()
                    message(WARNING \"Failed to sign app bundle\")
                endif()
            endif()
        endif()
        
        # Ensure the launcher executable and script have executable permissions
        if(EXISTS \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/MacOS/kolosal-launcher\")
            execute_process(COMMAND chmod +x \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/MacOS/kolosal-launcher\")
        endif()
        if(EXISTS \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/MacOS/kolosal-launcher.sh\")
            execute_process(COMMAND chmod +x \"\${ACTUAL_INSTALL_PREFIX}/Kolosal.app/Contents/MacOS/kolosal-launcher.sh\")
        endif()
    " COMPONENT Runtime)

elseif(UNIX)
    # Linux-specific installation
    # Install the main executable (now named kolosal)
    install(TARGETS kolosal
        RUNTIME DESTINATION bin
    )
    
    # Install the kolosal server executable and library with proper destinations
    install(TARGETS kolosal_server_exe
        RUNTIME DESTINATION bin
    )
    
    # In minimal DEB mode, ship the shared library in bin alongside executables (resolved via $ORIGIN)
    if(KOLOSAL_DEB_MINIMAL)
        install(TARGETS kolosal_server
            LIBRARY DESTINATION bin
        )
    else()
        install(TARGETS kolosal_server
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
        )
    endif()
    
    # Install the inference engine shared library
    if(BUILD_INFERENCE_ENGINE)
        if(KOLOSAL_DEB_MINIMAL)
            install(TARGETS ${INFERENCE_TARGET_NAME}
                LIBRARY DESTINATION bin
            )
        else()
            install(TARGETS ${INFERENCE_TARGET_NAME}
                LIBRARY DESTINATION lib
                ARCHIVE DESTINATION lib
            )
        endif()
    endif()
    
    # Bundle PoDoFo runtime dependencies so users don't need to install them
    # In minimal DEB mode, place them directly in bin so only bin/ is packaged
    if(KOLOSAL_DEB_MINIMAL)
        set(KOLOSAL_PRIVATE_LIBDIR "bin")
    else()
        set(KOLOSAL_PRIVATE_LIBDIR "lib/kolosal")
    endif()

    # Candidate SONAMEs commonly used on Debian/Ubuntu
    set(_KOLOSAL_PO_RUNTIME_SONAMES
        libssl.so.3
        libcrypto.so.3
        libfreetype.so.6
        libpng16.so.16
        libtiff.so.6
        libjpeg.so.8
        libjpeg.so.9
        libzstd.so.1
        liblzma.so.5
        libz.so.1
        libxml2.so.2
        libfontconfig.so.1
    )

    # Common library search roots across Debian/Ubuntu architectures
    set(_KOLOSAL_LIB_ROOTS
        /usr/lib
        /lib
        /usr/lib/x86_64-linux-gnu
        /lib/x86_64-linux-gnu
        /usr/lib/aarch64-linux-gnu
        /lib/aarch64-linux-gnu
        /usr/lib/arm-linux-gnueabihf
        /lib/arm-linux-gnueabihf
        /usr/lib/arm64-linux-gnu
        /lib/arm64-linux-gnu
    )

    # Try to install each runtime library if present on the build system
    foreach(_soname ${_KOLOSAL_PO_RUNTIME_SONAMES})
        set(_found_path "")
        foreach(_root ${_KOLOSAL_LIB_ROOTS})
            if(EXISTS "${_root}/${_soname}")
                set(_found_path "${_root}/${_soname}")
                break()
            endif()
        endforeach()
        if(_found_path)
            message(STATUS "Bundling runtime library: ${_found_path}")
            install(FILES "${_found_path}"
                DESTINATION ${KOLOSAL_PRIVATE_LIBDIR}
                PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
                OPTIONAL)
        else()
            message(STATUS "Runtime library not found (skipping): ${_soname}")
        endif()
    endforeach()

    # Install additional inference engine shared libraries if they exist (regardless of build config)
    set(INFERENCE_SO_CANDIDATES
        "${CMAKE_BINARY_DIR}/lib/libllama-vulkan.so"
        "${CMAKE_BINARY_DIR}/lib/libllama-cuda.so"
        "${CMAKE_BINARY_DIR}/lib/libllama-cpu.so"
        "${CMAKE_BINARY_DIR}/bin/libllama-vulkan.so"
        "${CMAKE_BINARY_DIR}/bin/libllama-cuda.so"
        "${CMAKE_BINARY_DIR}/bin/libllama-cpu.so"
        "${CMAKE_BINARY_DIR}/kolosal-server/lib/libllama-vulkan.so"
        "${CMAKE_BINARY_DIR}/kolosal-server/lib/libllama-cuda.so"
        "${CMAKE_BINARY_DIR}/kolosal-server/lib/libllama-cpu.so"
    )
    
    foreach(INFERENCE_SO_PATH ${INFERENCE_SO_CANDIDATES})
        if(KOLOSAL_DEB_MINIMAL)
            set(_inference_dest bin)
        else()
            set(_inference_dest lib)
        endif()
        install(FILES "${INFERENCE_SO_PATH}"
            DESTINATION ${_inference_dest}
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
            OPTIONAL
        )
    endforeach()
    
    # Install configuration: in minimal mode, ship config.yaml next to binaries; otherwise use /etc
    if(KOLOSAL_DEB_MINIMAL)
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/config.linux.yaml"
            DESTINATION bin
            RENAME config.yaml
            PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
        )
    else()
        # Install configuration file to etc/kolosal directory (use config.linux.yaml directly for reliable packaging)
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/config.linux.yaml"
            DESTINATION /etc/kolosal
            RENAME config.yaml
            PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
        )
        # Create the kolosal configuration directory during install
        install(DIRECTORY DESTINATION /etc/kolosal
            DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        )
    endif()
    
    # Install systemd service file for kolosal-server if it exists
    if(NOT KOLOSAL_DEB_MINIMAL)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/kolosal-server/kolosal-server.service")
            install(FILES kolosal-server/kolosal-server.service
                DESTINATION etc/systemd/system
                OPTIONAL
            )
        endif()
    endif()
    
    # Create a desktop entry (optional)
    if(NOT KOLOSAL_DEB_MINIMAL)
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
    
    # Install man page if it exists
    if(NOT KOLOSAL_DEB_MINIMAL)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/docs/kolosal.1")
            install(FILES docs/kolosal.1
                DESTINATION share/man/man1
                OPTIONAL
            )
        endif()
    endif()

    # Prune development and example artifacts from the install tree before packaging (always run)
    install(CODE "
        message(STATUS \"Pruning non-runtime files from Linux package...\")
        if(DEFINED ENV{DESTDIR})
            set(ROOT \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}\")
        else()
            set(ROOT \"${CMAKE_INSTALL_PREFIX}\")
        endif()

        # Some generators may keep files under ROOT/usr; normalize if needed
        if(NOT EXISTS \"${ROOT}/bin\" AND EXISTS \"${ROOT}/usr/bin\")
            set(ROOT \"${ROOT}/usr\")
        endif()

        # Remove Python helpers (*.py)
        file(GLOB PY_FILES \"${ROOT}/bin/*.py\")
        if(PY_FILES)
            file(REMOVE ${PY_FILES})
        endif()

        # Remove smoketest and test binaries
        if(EXISTS \"${ROOT}/bin/inference_smoketest\")
            file(REMOVE \"${ROOT}/bin/inference_smoketest\")
        endif()
        file(GLOB TEST_BINS \"${ROOT}/bin/test_*\")
        if(TEST_BINS)
            file(REMOVE ${TEST_BINS})
        endif()
        # Remove example binaries directory and any example executables
        if(EXISTS \"${ROOT}/bin/examples\")
            file(REMOVE_RECURSE \"${ROOT}/bin/examples\")
        endif()
        file(GLOB EXAMPLE_BINS \"${ROOT}/bin/*example*\")
        if(EXAMPLE_BINS)
            file(REMOVE ${EXAMPLE_BINS})
        endif()

        # Remove development headers (*.h)
        if(EXISTS \"${ROOT}/include\")
            file(REMOVE_RECURSE \"${ROOT}/include\")
        endif()

        # Remove static libraries (*.a)
        file(GLOB_RECURSE STATIC_LIBS \"${ROOT}/lib/*.a\")
        if(STATIC_LIBS)
            file(REMOVE ${STATIC_LIBS})
        endif()

        # Remove CMake package metadata and pkgconfig files
        if(EXISTS \"${ROOT}/lib/cmake\")
            file(REMOVE_RECURSE \"${ROOT}/lib/cmake\")
        endif()
        if(EXISTS \"${ROOT}/lib/pkgconfig\")
            file(REMOVE_RECURSE \"${ROOT}/lib/pkgconfig\")
        endif()

        # Remove example sources/data under share (contains .cpp, README, etc.)
        if(EXISTS \"${ROOT}/share/kolosal/examples\")
            file(REMOVE_RECURSE \"${ROOT}/share/kolosal/examples\")
        endif()

        message(STATUS \"Prune complete.\")
    ")
endif()

# CPack configuration for packaging
# Platform-specific generator selection
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
elseif(APPLE)
    # Use DragNDrop for DMG creation on macOS, but we'll customize it
    set(CPACK_GENERATOR "DragNDrop")
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

# Debian-specific: keep package lean in minimal mode
if(UNIX AND NOT APPLE)
    # Avoid pulling in system dpkg-shlibdeps scanning which can add many Depends
    if(KOLOSAL_DEB_MINIMAL)
        set(CPACK_DEBIAN_ENABLE_SHLIBDEPS OFF)
        # Provide a conservative dependency baseline, or leave empty to let admins resolve
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
    endif()
    # Install under /usr for DEB to avoid /usr/local default
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
    # Ensure prune runs during CPack staging as a last step
    set(CPACK_INSTALL_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/prune_linux_package.cmake")
endif()

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
        
        ; Create user config directory and copy config.yaml for writable access
        DetailPrint \\\"Setting up user configuration directory...\\\"
        
        ; Get the user's AppData\\\\Roaming directory explicitly
        ReadEnvStr \$3 \\\"USERPROFILE\\\"
        StrCpy \$4 \\\"\$3\\\\AppData\\\\Roaming\\\\Kolosal\\\"
        
        ; Create the directory
        CreateDirectory \\\"\$4\\\"
        
        ; Check if config already exists
        IfFileExists \\\"\$4\\\\config.yaml\\\" config_exists 0
        
        ; Copy config file to user directory
        CopyFiles \\\"\$INSTDIR\\\\config.yaml\\\" \\\"\$4\\\\config.yaml\\\"
        DetailPrint \\\"Configuration file copied to \$4\\\\config.yaml\\\"
        Goto config_done
        
        config_exists:
        DetailPrint \\\"User configuration already exists at \$4\\\\config.yaml, preserving existing settings\\\"
        
        config_done:
        
        ; Broadcast environment change to notify all applications
        SendMessage \\\${HWND_BROADCAST} \\\${WM_WININICHANGE} 0 \\\"STR:Environment\\\" /TIMEOUT=5000
    ")
    
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
        ; Remove from PATH using PowerShell
        StrCmp \\\$DO_NOT_ADD_TO_PATH \\\"1\\\" skip_custom_unpath
        
        DetailPrint \\\"Removing \$INSTDIR\\\\bin from user PATH using PowerShell...\\\"
        nsExec::ExecToLog 'powershell.exe -ExecutionPolicy Bypass -Command \\\"\$currentPath = [Environment]::GetEnvironmentVariable('PATH', 'User'); if (\$currentPath) { \$pathEntries = \$currentPath -split ';' | Where-Object { \$_.Trim() -ne '\$INSTDIR\\\\bin' -and \$_.Trim() -ne '' }; \$newPath = \$pathEntries -join ';'; [Environment]::SetEnvironmentVariable('PATH', \$newPath, 'User'); }\\\"'
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
- Inference Engine libraries (llama-vulkan.dll, llama-cuda.dll, llama-cpu.dll as available)
- Required dependencies (libcurl.dll, etc.)
- Configuration file (config.yaml)

The installation will:
- Install to your user profile directory
- Add the bin directory to your user PATH
- Create App Paths registry entries for easier access
- Copy config.yaml to %USERPROFILE%\\\\AppData\\\\Roaming\\\\Kolosal\\\\ for writable access
- Configure the server to use the writable config file
")
    
    # Architecture detection for Windows
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CPACK_SYSTEM_NAME "win64")
    else()
        set(CPACK_SYSTEM_NAME "win32")
    endif()
elseif(APPLE)
    # macOS-specific packaging settings for DMG distribution
    set(CPACK_DMG_VOLUME_NAME "Kolosal")
    
    # Use DragNDrop generator but override with our custom DMG creation
    set(CPACK_GENERATOR "DragNDrop")
    
    # Basic DMG settings 
    set(CPACK_DMG_FORMAT "UDBZ")  # Compressed DMG format
    
    # DMG background image
    set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/resources/dmg_background.png")
    
    # DMG window and icon configuration
    set(CPACK_DMG_WINDOW_X 100)
    set(CPACK_DMG_WINDOW_Y 100) 
    set(CPACK_DMG_WINDOW_WIDTH 640)
    set(CPACK_DMG_WINDOW_HEIGHT 480)
    set(CPACK_DMG_ICON_SIZE 128)
    
    # Package metadata
    set(CPACK_PACKAGE_NAME "Kolosal")
    set(CPACK_PACKAGE_FILE_NAME "kolosal-${PROJECT_VERSION}-macos")
    
    # Set install prefix for DMG contents - create app bundle structure
    set(CMAKE_INSTALL_PREFIX "/tmp/kolosal-dmg-staging" CACHE PATH "Install path prefix for DMG" FORCE)
    
    # Architecture detection for macOS
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(CPACK_SYSTEM_NAME "macos-arm64")
        set(CPACK_PACKAGE_FILE_NAME "kolosal-${PROJECT_VERSION}-apple-silicon")
    else()
        set(CPACK_SYSTEM_NAME "macos-x64")
        set(CPACK_PACKAGE_FILE_NAME "kolosal-${PROJECT_VERSION}-intel-mac")
    endif()
    
    # Components for macOS packaging
    set(CPACK_COMPONENTS_ALL Runtime)
    set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Kolosal")
    set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Main executable, server components, and all dependencies")
    set(CPACK_COMPONENT_RUNTIME_REQUIRED TRUE)
    
    # Code signing for CPack-generated DMG
    if(ENABLE_CODESIGN AND NOT CODESIGN_IDENTITY STREQUAL "")
        # Add post-build DMG signing (use Application certificate for DMG, not Installer)
        set(CPACK_POST_BUILD_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/sign_dmg.cmake")
        
        # Create the signing script
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/sign_dmg.cmake"
"# Sign the generated DMG with Application certificate (not Installer)
message(STATUS \"Signing DMG with identity: ${CODESIGN_IDENTITY}\")
execute_process(
    COMMAND codesign --force --sign \"${CODESIGN_IDENTITY}\" --timestamp \"\${CPACK_PACKAGE_FILE}\"
    RESULT_VARIABLE SIGN_RESULT
)

if(SIGN_RESULT EQUAL 0)
    message(STATUS \"DMG signed successfully\")
    
    # Verify signature
    execute_process(
        COMMAND codesign --verify --verbose \"\${CPACK_PACKAGE_FILE}\"
        RESULT_VARIABLE VERIFY_RESULT
    )
    
    if(VERIFY_RESULT EQUAL 0)
        message(STATUS \"DMG signature verified\")
    else()
        message(WARNING \"DMG signature verification failed\")
    endif()
    
    # Submit for notarization if team ID is provided
    if(NOT \"${DEVELOPER_TEAM_ID}\" STREQUAL \"\")
        message(STATUS \"Submitting DMG for notarization with team ID: ${DEVELOPER_TEAM_ID}\")
        
        # Try with stored credentials profile first
        execute_process(
            COMMAND xcrun notarytool submit \"\${CPACK_PACKAGE_FILE}\" --keychain-profile \"kolosal-profile\" --wait
            RESULT_VARIABLE NOTARIZE_RESULT
            ERROR_QUIET
        )
        
        if(NOT NOTARIZE_RESULT EQUAL 0)
            message(STATUS \"Stored credentials not found, trying with team-id authentication...\")
            execute_process(
                COMMAND xcrun notarytool submit \"\${CPACK_PACKAGE_FILE}\" --team-id \"${DEVELOPER_TEAM_ID}\" --wait
                RESULT_VARIABLE NOTARIZE_RESULT
            )
        endif()
        
        if(NOTARIZE_RESULT EQUAL 0)
            message(STATUS \"Notarization successful\")
            
            # Staple the notarization
            execute_process(
                COMMAND xcrun stapler staple \"\${CPACK_PACKAGE_FILE}\"
                RESULT_VARIABLE STAPLE_RESULT
            )
            
            if(STAPLE_RESULT EQUAL 0)
                message(STATUS \"Notarization stapled successfully\")
            else()
                message(WARNING \"Failed to staple notarization\")
            endif()
        else()
            message(WARNING \"Notarization failed. To set up credentials, run:\")
            message(WARNING \"  xcrun notarytool store-credentials 'kolosal-profile' --apple-id 'your-apple-id@example.com' --team-id '${DEVELOPER_TEAM_ID}' --password 'app-specific-password'\")
        endif()
    else()
        message(STATUS \"No team ID provided, skipping notarization\")
    endif()
else()
    message(WARNING \"Failed to sign DMG\")
endif()
")
    endif()
    
    # Add a custom target for creating DMG with complete signing workflow
    add_custom_target(dmg
        DEPENDS kolosal kolosal_server kolosal_server_exe kolosal-launcher
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/cmake/complete_signing.sh"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Creating DMG with complete signing workflow"
        VERBATIM
    )
    
    # Add message about packaging
    message(STATUS "macOS DMG packaging configured.")
    if(ENABLE_CODESIGN)
        if(NOT CODESIGN_IDENTITY STREQUAL "")
            message(STATUS "• Code signing enabled for applications with: ${CODESIGN_IDENTITY}")
        endif()
        if(NOT CODESIGN_INSTALLER_IDENTITY STREQUAL "")
            message(STATUS "• Code signing enabled for installers with: ${CODESIGN_INSTALLER_IDENTITY}")
        endif()
        if(NOT DEVELOPER_TEAM_ID STREQUAL "")
            message(STATUS "• Notarization enabled with Team ID: ${DEVELOPER_TEAM_ID}")
        endif()
    else()
        message(STATUS "• Code signing disabled (use -DENABLE_CODESIGN=ON to enable)")
    endif()
    message(STATUS "• Use 'make package' for standard DMG creation")
    message(STATUS "• Use 'make dmg' for DMG with custom background image")
    message(STATUS "• Double-clicking Kolosal.app will offer to add kolosal/kolosal-server to PATH")
    
else()
    # DEB package specific settings (Linux)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Kolosal Project <dev@kolosal.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    
    # Essential runtime dependencies based on ldd output
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.17), libgcc-s1, libstdc++6, libcurl4, libgomp1, libssl3 | libssl1.1, zlib1g")
    
    # Build suggests list properly
    if(DEBIAN_SUGGESTS_LIST)
        set(CPACK_DEBIAN_PACKAGE_SUGGESTS "${DEBIAN_SUGGESTS_LIST}")
    else()
        set(CPACK_DEBIAN_PACKAGE_SUGGESTS "")
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

# Ensure config file has proper permissions
if [ -f /etc/kolosal/config.yaml ]; then
    chmod 644 /etc/kolosal/config.yaml
    echo \"Configuration file found at /etc/kolosal/config.yaml\"
else
    echo \"Warning: Configuration file not found at /etc/kolosal/config.yaml\"
    echo \"This may indicate an installation issue.\"
fi

# Update library cache to include the new shared libraries
if command -v ldconfig >/dev/null 2>&1; then
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

# Handle additional inference engine libraries that may exist
for inference_engine in llama-vulkan llama-cuda llama-cpu; do
    INFERENCE_LIB_PATH=\"/usr/lib/lib\$inference_engine.so\"
    if [ -f \"\$INFERENCE_LIB_PATH\" ] && [ ! -e \"/usr/lib/x86_64-linux-gnu/lib\$inference_engine.so\" ]; then
        ln -sf \"\$INFERENCE_LIB_PATH\" \"/usr/lib/x86_64-linux-gnu/lib\$inference_engine.so\" 2>/dev/null || true
        echo \"Created symlink for \$inference_engine\"
    fi
done

# Create kolosal user and group if they don't exist (for systemd service)
if ! id \"kolosal\" >/dev/null 2>&1; then
    useradd --system --home-dir /var/lib/kolosal --shell /bin/false kolosal || true
fi

# Add current user to kolosal group if possible (for CLI access)
if [ \"\$SUDO_USER\" ]; then
    usermod -a -G kolosal \"\$SUDO_USER\" 2>/dev/null || true
fi

# Create kolosal directories
if [ ! -d /var/lib/kolosal ]; then
    mkdir -p /var/lib/kolosal
    chmod 755 /var/lib/kolosal
fi

if [ ! -d /var/lib/kolosal/models ]; then
    mkdir -p /var/lib/kolosal/models
    chmod 755 /var/lib/kolosal/models
fi

# Set ownership and permissions
if id \"kolosal\" >/dev/null 2>&1; then
    chown kolosal:kolosal /var/lib/kolosal
    chown kolosal:kolosal /var/lib/kolosal/models
    chmod 775 /var/lib/kolosal
    chmod 775 /var/lib/kolosal/models
fi

# Set up systemd service if available
if command -v systemctl >/dev/null 2>&1 && [ -f /etc/systemd/system/kolosal-server.service ]; then
    systemctl daemon-reload
    systemctl enable kolosal-server 2>/dev/null || true
    systemctl start kolosal-server 2>/dev/null || true
fi

echo \"Kolosal CLI installed successfully!\"
echo \"Configuration: /etc/kolosal/config.yaml\"
echo \"Data directory: /var/lib/kolosal\"
echo \"\"
echo \"Getting started:\"
echo \"  kolosal --help\"
echo \"  kolosal-server --help\"

exit 0
")
    # Ensure our private lib dir is preferred at runtime by executables and shared libs
    # The targets were built with INSTALL_RPATH including $ORIGIN/../lib/kolosal, so ld.so will find these first.
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

# Remove additional inference engine symlinks
for inference_engine in llama-vulkan llama-cuda llama-cpu; do
    rm -f \"/usr/lib/x86_64-linux-gnu/lib\$inference_engine.so\" 2>/dev/null || true
done

echo \"Kolosal CLI has been removed.\"
echo \"User data in /var/lib/kolosal has been preserved.\"
echo \"To completely remove all data: sudo rm -rf /var/lib/kolosal /etc/kolosal && sudo userdel kolosal\"

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
    
    # Essential runtime dependencies based on ldd output
    set(CPACK_RPM_PACKAGE_REQUIRES "glibc >= 2.17, libgcc, libstdc++, libcurl, libgomp, openssl-libs, zlib")
    
    # Add acceleration-specific dependencies
    if(USE_CUDA)
        set(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, cuda-runtime")
        set(RPM_SUGGESTS_LIST "vulkan-loader")
    elseif(USE_VULKAN)
        set(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, vulkan-loader")
        set(RPM_SUGGESTS_LIST "")
    else()
        # Always suggest vulkan runtime since we include vulkan inference engine by default
        set(RPM_SUGGESTS_LIST "vulkan-loader")
    endif()
    
    # Set RPM suggests (always include since we package all available inference engines)
    set(CPACK_RPM_PACKAGE_SUGGESTS "${RPM_SUGGESTS_LIST}")
    
    # RPM post-install and pre-uninstall scripts
    set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${POSTINST_SCRIPT}")
    set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${PRERM_SCRIPT}")
    set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${POSTRM_SCRIPT}")
    
    # Check if we should build RPM instead of DEB
    if(EXISTS "/etc/redhat-release" OR EXISTS "/etc/fedora-release")
        set(CPACK_GENERATOR "RPM")
    endif()
endif()

include(CPack)