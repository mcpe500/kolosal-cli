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
option(USE_METAL "Enable Apple Metal support" OFF)
option(ENABLE_NATIVE_OPTIMIZATION "Enable native CPU optimization (-march=native)" OFF)

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

# Create launcher script for macOS during build
if(APPLE)
    add_custom_command(TARGET kolosal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Creating kolosal-launcher script"
        COMMENT "Creating macOS launcher script"
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
        "${CMAKE_BINARY_DIR}/bin/libkolosal_server.dylib"
        COMMENT "Copying libkolosal_server.dylib to output directory"
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
            "${CMAKE_BINARY_DIR}/bin/lib${INFERENCE_TARGET_NAME}.dylib"
            COMMENT "Copying lib${INFERENCE_TARGET_NAME}.dylib to output directory"
            DEPENDS ${INFERENCE_TARGET_NAME}
        )
    endif()
    
    # Set RPATH for runtime library search in the same directory and standard macOS paths
    set_target_properties(kolosal PROPERTIES
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
    set_target_properties(kolosal PROPERTIES
        INSTALL_RPATH "$ORIGIN:/usr/lib"
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
    # macOS-specific installation - Bundle structure for .app
    # Install executables to MacOS directory (bundle structure)
    install(TARGETS kolosal
        RUNTIME DESTINATION MacOS
        COMPONENT Runtime
    )
    
    install(TARGETS kolosal_server_exe
        RUNTIME DESTINATION MacOS
        COMPONENT Runtime
    )
    
    # Install shared libraries to Frameworks directory (bundle structure)
    install(TARGETS kolosal_server
        LIBRARY DESTINATION Frameworks
        ARCHIVE DESTINATION Frameworks
        COMPONENT Runtime
    )
    
    # Install the inference engine shared library
    if(BUILD_INFERENCE_ENGINE)
        install(TARGETS ${INFERENCE_TARGET_NAME}
            LIBRARY DESTINATION Frameworks
            ARCHIVE DESTINATION Frameworks
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
    )
    
    foreach(INFERENCE_DYLIB_PATH ${INFERENCE_DYLIB_CANDIDATES})
        install(FILES "${INFERENCE_DYLIB_PATH}"
            DESTINATION Frameworks
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
            COMPONENT Runtime
            OPTIONAL
        )
    endforeach()
    
    # Install configuration file to Resources directory (bundle structure)
    install(FILES "${CMAKE_BINARY_DIR}/bin/config.yaml"
        DESTINATION Resources
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
        COMPONENT Runtime
    )
    
    # Install the launcher script to MacOS directory
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/kolosal-launcher"
        DESTINATION MacOS
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT Runtime
    )
    
    # Install man page if it exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/docs/kolosal.1")
        install(FILES docs/kolosal.1
            DESTINATION Resources/man/man1
            COMPONENT Runtime
            OPTIONAL
        )
    endif()
    
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
        install(FILES "${INFERENCE_SO_PATH}"
            DESTINATION lib
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
            OPTIONAL
        )
    endforeach()
    
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

# CPack configuration for packaging
# Platform-specific generator selection
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
elseif(APPLE)
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
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "Kolosal CLI")
    set(CPACK_DMG_FORMAT "UDBZ")  # Compressed DMG
    
    # Create symbolic link to Applications folder in DMG for easy installation
    set(CPACK_DMG_DISABLE_APPLICATIONS_SYMLINK OFF)
    
    # Bundle configuration
    set(CPACK_BUNDLE_NAME "Kolosal CLI")
    set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
    
    # Create a launcher script that sets up symlinks and handles terminal access
    set(LAUNCHER_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/kolosal-launcher.sh")
    file(WRITE ${LAUNCHER_SCRIPT}
"#!/bin/bash
# Kolosal CLI Launcher Script
# This script sets up command-line access and launches the terminal interface

APP_DIR=\"\$(dirname \"\$0\")\"
BUNDLE_DIR=\"\$(dirname \"\$(dirname \"\$APP_DIR\")\")\"
BIN_DIR=\"\$BUNDLE_DIR/Contents/MacOS\"

# Function to create symlinks with user permission
setup_symlinks() {
    # Check if /usr/local/bin exists, create if it doesn't
    if [ ! -d \"/usr/local/bin\" ]; then
        echo \"Creating /usr/local/bin directory...\"
        sudo mkdir -p /usr/local/bin 2>/dev/null || {
            echo \"Note: Could not create /usr/local/bin. You may need to run with admin privileges.\"
            return 1
        }
    fi
    
    # Create symlinks for kolosal and kolosal-server
    if [ -f \"\$BIN_DIR/kolosal\" ]; then
        echo \"Creating symlink for kolosal command...\"
        sudo ln -sf \"\$BIN_DIR/kolosal\" /usr/local/bin/kolosal 2>/dev/null || {
            echo \"Note: Could not create symlink in /usr/local/bin. You can manually add \$BIN_DIR to your PATH.\"
        }
    fi
    
    if [ -f \"\$BIN_DIR/kolosal-server\" ]; then
        echo \"Creating symlink for kolosal-server command...\"
        sudo ln -sf \"\$BIN_DIR/kolosal-server\" /usr/local/bin/kolosal-server 2>/dev/null || {
            echo \"Note: Could not create symlink in /usr/local/bin. You can manually add \$BIN_DIR to your PATH.\"
        }
    fi
}

# Function to update shell profiles
update_shell_profiles() {
    local shell_profiles=(\"\$HOME/.zshrc\" \"\$HOME/.bash_profile\" \"\$HOME/.bashrc\" \"\$HOME/.profile\")
    local path_added=false
    
    for profile in \"\${shell_profiles[@]}\"; do
        if [ -f \"\$profile\" ]; then
            # Check if PATH is already configured
            if ! grep -q \"/usr/local/bin\" \"\$profile\" 2>/dev/null && ! grep -q \"\$BIN_DIR\" \"\$profile\" 2>/dev/null; then
                echo \"# Added by Kolosal CLI\" >> \"\$profile\"
                echo \"export PATH=\\\"/usr/local/bin:\$BIN_DIR:\\\$PATH\\\"\" >> \"\$profile\"
                path_added=true
                echo \"Updated \$profile\"
            fi
        fi
    done
    
    if [ \"\$path_added\" = true ]; then
        echo \"Shell profiles updated. Please restart your terminal or run 'source ~/.zshrc' (or appropriate shell config).\"
    fi
}

# Main setup process
echo \"Setting up Kolosal CLI...\"

# Setup symlinks (may require admin password)
setup_symlinks

# Update shell profiles for current user
update_shell_profiles

# Create user directories
mkdir -p \"\$HOME/Library/Application Support/Kolosal\"
mkdir -p \"\$HOME/Library/Application Support/Kolosal/models\"
mkdir -p \"\$HOME/Library/Logs/Kolosal\"
mkdir -p \"\$HOME/Library/Caches/Kolosal/models\"

# Copy config file to user directory if it doesn't exist
if [ ! -f \"\$HOME/Library/Application Support/Kolosal/config.yaml\" ] && [ -f \"\$BUNDLE_DIR/Contents/Resources/config.yaml\" ]; then
    cp \"\$BUNDLE_DIR/Contents/Resources/config.yaml\" \"\$HOME/Library/Application Support/Kolosal/config.yaml\"
    echo \"Configuration file copied to ~/Library/Application Support/Kolosal/config.yaml\"
fi

echo \"\"
echo \"Kolosal CLI setup complete!\"
echo \"\"
echo \"You can now use:\"
echo \"  kolosal --help\"
echo \"  kolosal-server --help\"
echo \"\"
echo \"If the commands are not found, please restart your terminal or add \$BIN_DIR to your PATH manually.\"

# Keep the terminal open to show the setup messages
read -p \"Press Enter to continue...\"
")
    
    # Make the launcher executable
    file(COPY ${LAUNCHER_SCRIPT} 
         DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/kolosal-launcher"
         FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    
    # Create Info.plist for macOS bundle
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>
    <key>CFBundleExecutable</key>
    <string>kolosal-launcher</string>
    <key>CFBundleIdentifier</key>
    <string>com.kolosal.cli</string>
    <key>CFBundleName</key>
    <string>Kolosal CLI</string>
    <key>CFBundleVersion</key>
    <string>${PROJECT_VERSION}</string>
    <key>CFBundleShortVersionString</key>
    <string>${PROJECT_VERSION}</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.13</string>
    <key>LSRequiresNativeExecution</key>
    <true/>
    <key>NSHumanReadableCopyright</key>
    <string>Copyright © 2025 Kolosal Project. All rights reserved.</string>
    <key>LSApplicationCategoryType</key>
    <string>public.app-category.developer-tools</string>
    <key>NSAppleScriptEnabled</key>
    <true/>
    <key>LSUIElement</key>
    <false/>
    <key>LSBackgroundOnly</key>
    <false/>
</dict>
</plist>")
    
    # Add macOS-specific package information
    set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    
    # DMG customization
    set(CPACK_DMG_VOLUME_NAME "Install Kolosal CLI")
    set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/DMGSetup.scpt")
    
    # Create a simple AppleScript for DMG setup
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/DMGSetup.scpt"
"tell application \"Finder\"
    tell disk \"Install Kolosal CLI\"
        open
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        set the bounds of container window to {400, 100, 920, 440}
        set viewOptions to the icon view options of container window
        set arrangement of viewOptions to not arranged
        set icon size of viewOptions to 72
        make new alias file at container window to POSIX file \"/Applications\" with properties {name:\"Applications\"}
        set position of item \"Kolosal CLI.app\" of container window to {160, 205}
        set position of item \"Applications\" of container window to {360, 205}
        close
        open
        update without registering applications
        delay 2
    end tell
end tell")
    
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
    set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Kolosal CLI Runtime")
    set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Main executable, server components, and all dependencies")
    set(CPACK_COMPONENT_RUNTIME_REQUIRED TRUE)
    
else()
    # DEB package specific settings (Linux)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Kolosal Project <dev@kolosal.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    
    # Essential runtime dependencies based on ldd output
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.17), libgcc-s1, libstdc++6, libcurl4, libgomp1, libssl3 | libssl1.1, zlib1g")
    
    # Add acceleration-specific dependencies
    if(USE_CUDA)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, nvidia-cuda-runtime | cuda-runtime")
        set(DEBIAN_SUGGESTS_LIST "libvulkan1")
    elseif(USE_VULKAN)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libvulkan1")
        set(DEBIAN_SUGGESTS_LIST "")
    else()
        # Always suggest vulkan runtime since we include vulkan inference engine by default
        set(DEBIAN_SUGGESTS_LIST "libvulkan1")
    endif()
    
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
    echo "Configuration file found at /etc/kolosal/config.yaml"
else
    echo "Warning: Configuration file not found at /etc/kolosal/config.yaml"
    echo "This may indicate an installation issue."
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