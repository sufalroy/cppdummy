include("${CMAKE_CURRENT_LIST_DIR}/Utilities.cmake")

macro(detect_architecture)
    if (NOT CMAKE_SYSTEM_PROCESSOR)
        message(WARNING "CMAKE_SYSTEM_PROCESSOR is not set, using CMAKE_HOST_SYSTEM_PROCESSOR")
        set(CMAKE_SYSTEM_PROCESSOR "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    endif ()

    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" CMAKE_SYSTEM_PROCESSOR_LOWER)

    if (CMAKE_SYSTEM_PROCESSOR_LOWER MATCHES "^(x86|i[3-6]86)$")
        set(VCVARSALL_ARCH x86)
    elseif (CMAKE_SYSTEM_PROCESSOR_LOWER MATCHES "^(x64|x86_64|amd64)$")
        set(VCVARSALL_ARCH x64)
    elseif (CMAKE_SYSTEM_PROCESSOR_LOWER STREQUAL "arm")
        set(VCVARSALL_ARCH arm)
    elseif (CMAKE_SYSTEM_PROCESSOR_LOWER MATCHES "^(arm64|aarch64)$")
        set(VCVARSALL_ARCH arm64)
    else ()
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(VCVARSALL_ARCH x64)
            message(STATUS "Unknown architecture '${CMAKE_SYSTEM_PROCESSOR_LOWER}', assuming x64 based on pointer size")
        else ()
            set(VCVARSALL_ARCH x86)
            message(STATUS "Unknown architecture '${CMAKE_SYSTEM_PROCESSOR_LOWER}', assuming x86 based on pointer size")
        endif ()
    endif ()

    message(STATUS "Detected architecture: ${VCVARSALL_ARCH}")
endmacro()

function(run_vcvarsall)
    # Skip if not MSVC or if already configured
    if (NOT MSVC)
        return()
    endif ()

    # Check if vcvarsall has already been run
    if (NOT "$ENV{VSCMD_VER}" STREQUAL "")
        message(STATUS "MSVC environment already configured (VSCMD_VER=$ENV{VSCMD_VER})")
        return()
    endif ()

    # Cache the vcvarsall file location
    if (NOT VCVARSALL_FILE)
        # Find vcvarsall.bat with improved search paths
        get_filename_component(MSVC_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)

        # More comprehensive search paths
        set(SEARCH_PATHS
                "${MSVC_DIR}"
                "${MSVC_DIR}/.."
                "${MSVC_DIR}/../.."
                "${MSVC_DIR}/../../.."
                "${MSVC_DIR}/../../../.."
                "${MSVC_DIR}/../../../../.."
                "${MSVC_DIR}/../../../../../.."
                "${MSVC_DIR}/../../../../../../.."
                "${MSVC_DIR}/../../../../../../../.."
        )

        find_file(
                VCVARSALL_FILE
                NAMES vcvarsall.bat
                PATHS ${SEARCH_PATHS}
                PATH_SUFFIXES
                "VC/Auxiliary/Build"
                "Common7/Tools"
                "Tools"
                "VC"
                CACHE
        )
    endif ()

    if (NOT EXISTS "${VCVARSALL_FILE}")
        message(WARNING
                "Could not find vcvarsall.bat for automatic MSVC environment preparation.\n"
                "Please manually open the MSVC command prompt and rebuild the project.\n"
                "Searched in: ${MSVC_DIR} and subdirectories")
        return()
    endif ()

    # Detect architecture
    detect_architecture()

    # Run vcvarsall and capture environment
    message(STATUS "Running '${VCVARSALL_FILE} ${VCVARSALL_ARCH}' to set up the MSVC environment")

    execute_process(
            COMMAND cmd /c "${VCVARSALL_FILE}" ${VCVARSALL_ARCH} && echo VCVARSALL_ENV_START && set
            OUTPUT_VARIABLE VCVARSALL_OUTPUT
            ERROR_VARIABLE VCVARSALL_ERROR
            RESULT_VARIABLE VCVARSALL_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (NOT VCVARSALL_RESULT EQUAL 0)
        message(WARNING "vcvarsall.bat failed with exit code ${VCVARSALL_RESULT}")
        if (VCVARSALL_ERROR)
            message(WARNING "Error output: ${VCVARSALL_ERROR}")
        endif ()
        return()
    endif ()

    # Parse the output and set environment variables
    find_substring_by_prefix(VCVARSALL_ENV "VCVARSALL_ENV_START" "${VCVARSALL_OUTPUT}")
    if (VCVARSALL_ENV)
        set_env_from_string("${VCVARSALL_ENV}")
        message(STATUS "MSVC environment configured successfully")
    else ()
        message(WARNING "Failed to parse vcvarsall.bat output")
    endif ()
endfunction()
