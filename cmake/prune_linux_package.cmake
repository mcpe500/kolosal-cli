# Prune non-runtime files from Linux package staging tree
message(STATUS "[CPack] Pruning non-runtime files from Linux package...")

# Determine staging root
if(DEFINED ENV{DESTDIR})
    set(_ROOT "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}")
else()
    set(_ROOT "${CMAKE_INSTALL_PREFIX}")
endif()

# Normalize for cases where content is under usr/
if(NOT EXISTS "${_ROOT}/bin" AND EXISTS "${_ROOT}/usr/bin")
    set(_ROOT "${_ROOT}/usr")
endif()

# Safety: if computed root doesn't exist yet (pre-install call), try to discover it
if(NOT EXISTS "${_ROOT}")
    # Try common candidates
    set(_CANDIDATES)
    list(APPEND _CANDIDATES "$ENV{DESTDIR}/usr")
    list(APPEND _CANDIDATES "$ENV{DESTDIR}/usr/local")
    list(APPEND _CANDIDATES "${CMAKE_BINARY_DIR}/_CPack_Packages/Linux/DEB")
    list(APPEND _CANDIDATES "${CMAKE_BINARY_DIR}/_CPack_Packages")

    set(_FOUND_ROOT "")
    foreach(_base ${_CANDIDATES})
        if(EXISTS "${_base}")
            # Look for directories that contain usr/bin or usr/local/bin
            file(GLOB_RECURSE _roots LIST_DIRECTORIES true "${_base}/*")
            foreach(_r ${_roots})
                if(EXISTS "${_r}/usr/bin")
                    set(_FOUND_ROOT "${_r}/usr")
                    break()
                elseif(EXISTS "${_r}/usr/local/bin")
                    set(_FOUND_ROOT "${_r}/usr/local")
                    break()
                endif()
            endforeach()
        endif()
        if(_FOUND_ROOT)
            break()
        endif()
    endforeach()

    if(_FOUND_ROOT)
        set(_ROOT "${_FOUND_ROOT}")
    else()
        message(WARNING "[CPack] Prune aborted: staging root not found: ${_ROOT}")
        return()
    endif()
endif()

# 1) Remove Python helpers (*.py)
file(GLOB _PY_FILES "${_ROOT}/bin/*.py")
if(_PY_FILES)
    file(REMOVE ${_PY_FILES})
endif()

# 2) Remove smoketest and test binaries
if(EXISTS "${_ROOT}/bin/inference_smoketest")
    file(REMOVE "${_ROOT}/bin/inference_smoketest")
endif()
file(GLOB _TEST_BINS "${_ROOT}/bin/test_*")
if(_TEST_BINS)
    file(REMOVE ${_TEST_BINS})
endif()
# Remove example binaries directory and any example executables
if(EXISTS "${_ROOT}/bin/examples")
    file(REMOVE_RECURSE "${_ROOT}/bin/examples")
endif()
file(GLOB _EXAMPLE_BINS "${_ROOT}/bin/*example*")
if(_EXAMPLE_BINS)
    file(REMOVE ${_EXAMPLE_BINS})
endif()

# 3) Remove development headers (*.h)
if(EXISTS "${_ROOT}/include")
    file(REMOVE_RECURSE "${_ROOT}/include")
endif()

# 4) Remove static libraries (*.a)
file(GLOB_RECURSE _STATIC_LIBS "${_ROOT}/lib/*.a")
if(_STATIC_LIBS)
    file(REMOVE ${_STATIC_LIBS})
endif()

# 5) Remove CMake package metadata and pkgconfig files
if(EXISTS "${_ROOT}/lib/cmake")
    file(REMOVE_RECURSE "${_ROOT}/lib/cmake")
endif()
if(EXISTS "${_ROOT}/lib/pkgconfig")
    file(REMOVE_RECURSE "${_ROOT}/lib/pkgconfig")
endif()

# 6) Remove example sources/data under share
if(EXISTS "${_ROOT}/share/kolosal/examples")
    file(REMOVE_RECURSE "${_ROOT}/share/kolosal/examples")
endif()

message(STATUS "[CPack] Prune complete: ${_ROOT}")
