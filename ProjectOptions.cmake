include(cmake/SystemLink.cmake)
include(cmake/LibFuzzer.cmake)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

macro(cppdummy_supports_sanitizers)
    set(SUPPORTS_UBSAN OFF)
    set(SUPPORTS_ASAN OFF)
    set(SUPPORTS_TSAN OFF)
    set(SUPPORTS_MSAN OFF)

    set(_saved_required_flags "${CMAKE_REQUIRED_FLAGS}")
    set(_saved_required_link_options "${CMAKE_REQUIRED_LINK_OPTIONS}")

    set(TEST_PROGRAM "int main() { return 0; }")

    if ((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)
        message(STATUS "Checking UndefinedBehaviorSanitizer support...")

        set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined")
        set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=undefined")
        check_cxx_source_compiles("${TEST_PROGRAM}" HAS_UBSAN_LINK_SUPPORT)

        if (HAS_UBSAN_LINK_SUPPORT)
            message(STATUS "UndefinedBehaviorSanitizer: supported")
            set(SUPPORTS_UBSAN ON)
        else ()
            message(WARNING "UndefinedBehaviorSanitizer: NOT supported at link time")
        endif ()
    else ()
        message(STATUS "UndefinedBehaviorSanitizer: not available on this platform/compiler")
    endif ()

    set(CMAKE_REQUIRED_FLAGS "${_saved_required_flags}")
    set(CMAKE_REQUIRED_LINK_OPTIONS "${_saved_required_link_options}")

    if (WIN32)
        if (MSVC)
            message(STATUS "AddressSanitizer: supported on Windows with MSVC")
            set(SUPPORTS_ASAN OFF)
        elseif (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*")
            message(STATUS "AddressSanitizer: supported on Windows with Clang")
            set(SUPPORTS_ASAN ON)
        else ()
            message(STATUS "AddressSanitizer: limited support on Windows with GCC")
        endif ()
    else ()
        if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*")
            message(STATUS "Checking AddressSanitizer support...")

            set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
            set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address")
            check_cxx_source_compiles("${TEST_PROGRAM}" HAS_ASAN_LINK_SUPPORT)

            if (HAS_ASAN_LINK_SUPPORT)
                message(STATUS "AddressSanitizer: supported")
                set(SUPPORTS_ASAN ON)
            else ()
                message(WARNING "AddressSanitizer: NOT supported at link time")
            endif ()
        else ()
            message(STATUS "AddressSanitizer: not available with this compiler")
        endif ()
    endif ()

    set(CMAKE_REQUIRED_FLAGS "${_saved_required_flags}")
    set(CMAKE_REQUIRED_LINK_OPTIONS "${_saved_required_link_options}")

    if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" AND NOT WIN32)
        message(STATUS "Checking ThreadSanitizer support...")

        set(CMAKE_REQUIRED_FLAGS "-fsanitize=thread")
        set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=thread")
        check_cxx_source_compiles("${TEST_PROGRAM}" HAS_TSAN_LINK_SUPPORT)

        if (HAS_TSAN_LINK_SUPPORT)
            message(STATUS "ThreadSanitizer: supported")
            set(SUPPORTS_TSAN ON)
        else ()
            message(WARNING "ThreadSanitizer: NOT supported at link time")
        endif ()
    else ()
        message(STATUS "ThreadSanitizer: not available on this platform/compiler")
    endif ()

    set(CMAKE_REQUIRED_FLAGS "${_saved_required_flags}")
    set(CMAKE_REQUIRED_LINK_OPTIONS "${_saved_required_link_options}")

    if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" AND NOT WIN32)
        message(STATUS "Checking MemorySanitizer support...")

        set(CMAKE_REQUIRED_FLAGS "-fsanitize=memory")
        set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=memory")
        check_cxx_source_compiles("${TEST_PROGRAM}" HAS_MSAN_LINK_SUPPORT)

        if (HAS_MSAN_LINK_SUPPORT)
            message(STATUS "MemorySanitizer: supported")
            set(SUPPORTS_MSAN ON)
        else ()
            message(WARNING "MemorySanitizer: NOT supported at link time")
        endif ()
    else ()
        message(STATUS "MemorySanitizer: not available on this platform/compiler")
    endif ()

    set(CMAKE_REQUIRED_FLAGS "${_saved_required_flags}")
    set(CMAKE_REQUIRED_LINK_OPTIONS "${_saved_required_link_options}")

    message(STATUS "Sanitizer support summary:")
    message(STATUS "  UndefinedBehaviorSanitizer: ${SUPPORTS_UBSAN}")
    message(STATUS "  AddressSanitizer: ${SUPPORTS_ASAN}")
    message(STATUS "  ThreadSanitizer: ${SUPPORTS_TSAN}")
    message(STATUS "  MemorySanitizer: ${SUPPORTS_MSAN}")
endmacro()

macro(cppdummy_setup_options)
    option(cppdummy_ENABLE_HARDENING "Enable hardening" ON)
    option(cppdummy_ENABLE_COVERAGE "Enable coverage reporting" OFF)

    cmake_dependent_option(
            cppdummy_ENABLE_GLOBAL_HARDENING
            "Attempt to push hardening options to built dependencies"
            ON
            cppdummy_ENABLE_HARDENING
            OFF)

    cppdummy_supports_sanitizers()

    if (NOT PROJECT_IS_TOP_LEVEL OR cppdummy_PACKAGING_MAINTAINER_MODE)
        option(cppdummy_ENABLE_IPO "Enable IPO/LTO" OFF)
        option(cppdummy_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
        option(cppdummy_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
        option(cppdummy_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
        option(cppdummy_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
        option(cppdummy_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
        option(cppdummy_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
        option(cppdummy_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
        option(cppdummy_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
        option(cppdummy_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
        option(cppdummy_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
        option(cppdummy_ENABLE_PCH "Enable precompiled headers" OFF)
        option(cppdummy_ENABLE_CACHE "Enable ccache" OFF)
    else ()
        option(cppdummy_ENABLE_IPO "Enable IPO/LTO" ON)
        option(cppdummy_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
        option(cppdummy_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
        option(cppdummy_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${SUPPORTS_ASAN})
        option(cppdummy_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
        option(cppdummy_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})
        option(cppdummy_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF) # Usually conflicts with ASAN
        option(cppdummy_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF) # Usually conflicts with other sanitizers
        option(cppdummy_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
        option(cppdummy_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
        option(cppdummy_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)
        option(cppdummy_ENABLE_PCH "Enable precompiled headers" OFF)
        option(cppdummy_ENABLE_CACHE "Enable ccache" ON)
    endif ()

    if (NOT PROJECT_IS_TOP_LEVEL)
        mark_as_advanced(
                cppdummy_ENABLE_IPO
                cppdummy_WARNINGS_AS_ERRORS
                cppdummy_ENABLE_USER_LINKER
                cppdummy_ENABLE_SANITIZER_ADDRESS
                cppdummy_ENABLE_SANITIZER_LEAK
                cppdummy_ENABLE_SANITIZER_UNDEFINED
                cppdummy_ENABLE_SANITIZER_THREAD
                cppdummy_ENABLE_SANITIZER_MEMORY
                cppdummy_ENABLE_UNITY_BUILD
                cppdummy_ENABLE_CLANG_TIDY
                cppdummy_ENABLE_CPPCHECK
                cppdummy_ENABLE_COVERAGE
                cppdummy_ENABLE_PCH
                cppdummy_ENABLE_CACHE)
    endif ()

    cppdummy_check_libfuzzer_support(LIBFUZZER_SUPPORTED)
    if (LIBFUZZER_SUPPORTED AND (cppdummy_ENABLE_SANITIZER_ADDRESS OR cppdummy_ENABLE_SANITIZER_THREAD OR cppdummy_ENABLE_SANITIZER_UNDEFINED))
        set(DEFAULT_FUZZER ON)
    else ()
        set(DEFAULT_FUZZER OFF)
    endif ()

    option(cppdummy_BUILD_FUZZ_TESTS "Enable fuzz testing executable" ${DEFAULT_FUZZER})

    if (cppdummy_ENABLE_SANITIZER_THREAD AND cppdummy_ENABLE_SANITIZER_ADDRESS)
        message(WARNING "ThreadSanitizer and AddressSanitizer are incompatible. Disabling ThreadSanitizer.")
        set(cppdummy_ENABLE_SANITIZER_THREAD OFF CACHE BOOL "Enable thread sanitizer" FORCE)
    endif ()

    if (cppdummy_ENABLE_SANITIZER_MEMORY AND (cppdummy_ENABLE_SANITIZER_ADDRESS OR cppdummy_ENABLE_SANITIZER_THREAD))
        message(WARNING "MemorySanitizer is incompatible with AddressSanitizer and ThreadSanitizer. Disabling MemorySanitizer.")
        set(cppdummy_ENABLE_SANITIZER_MEMORY OFF CACHE BOOL "Enable memory sanitizer" FORCE)
    endif ()
endmacro()

macro(cppdummy_global_options)
    if (cppdummy_ENABLE_IPO)
        include(cmake/InterproceduralOptimization.cmake)
        cppdummy_enable_ipo()
    endif ()

    cppdummy_supports_sanitizers()

    if (cppdummy_ENABLE_HARDENING AND cppdummy_ENABLE_GLOBAL_HARDENING)
        include(cmake/Hardening.cmake)

        if (NOT SUPPORTS_UBSAN
                OR cppdummy_ENABLE_SANITIZER_UNDEFINED
                OR cppdummy_ENABLE_SANITIZER_ADDRESS
                OR cppdummy_ENABLE_SANITIZER_THREAD
                OR cppdummy_ENABLE_SANITIZER_LEAK)
            set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
        else ()
            set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
        endif ()

        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            message(STATUS "Hardening configuration:")
            message(STATUS "  ENABLE_HARDENING: ${cppdummy_ENABLE_HARDENING}")
            message(STATUS "  ENABLE_UBSAN_MINIMAL_RUNTIME: ${ENABLE_UBSAN_MINIMAL_RUNTIME}")
            message(STATUS "  ENABLE_SANITIZER_UNDEFINED: ${cppdummy_ENABLE_SANITIZER_UNDEFINED}")
        endif ()

        cppdummy_enable_hardening(cppdummy_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
    endif ()
endmacro()

macro(cppdummy_local_options)
    if (PROJECT_IS_TOP_LEVEL)
        include(cmake/StandardProjectSettings.cmake)
    endif ()

    add_library(cppdummy_warnings INTERFACE)
    add_library(cppdummy_options INTERFACE)

    include(cmake/CompilerWarnings.cmake)
    cppdummy_set_project_warnings(
            cppdummy_warnings
            ${cppdummy_WARNINGS_AS_ERRORS}
            ""  # MSVC warnings
            ""  # Clang warnings
            ""  # GCC warnings
            "")  # CUDA warnings

    if (cppdummy_ENABLE_USER_LINKER)
        include(cmake/Linker.cmake)
        cppdummy_configure_linker(cppdummy_options)
    endif ()

    include(cmake/Sanitizers.cmake)
    cppdummy_enable_sanitizers(
            cppdummy_options
            ${cppdummy_ENABLE_SANITIZER_ADDRESS}
            ${cppdummy_ENABLE_SANITIZER_LEAK}
            ${cppdummy_ENABLE_SANITIZER_UNDEFINED}
            ${cppdummy_ENABLE_SANITIZER_THREAD}
            ${cppdummy_ENABLE_SANITIZER_MEMORY})

    set_target_properties(cppdummy_options PROPERTIES UNITY_BUILD ${cppdummy_ENABLE_UNITY_BUILD})

    if (cppdummy_ENABLE_PCH)
        target_precompile_headers(
                cppdummy_options
                INTERFACE
                <vector>
                <string>
                <utility>
                <memory>
                <algorithm>)
    endif ()

    if (cppdummy_ENABLE_CACHE)
        include(cmake/Cache.cmake)
        cppdummy_enable_cache()
    endif ()

    include(cmake/StaticAnalyzers.cmake)
    if (cppdummy_ENABLE_CLANG_TIDY)
        cppdummy_enable_clang_tidy(cppdummy_options ${cppdummy_WARNINGS_AS_ERRORS})
    endif ()

    if (cppdummy_ENABLE_CPPCHECK)
        cppdummy_enable_cppcheck(${cppdummy_WARNINGS_AS_ERRORS} "")
    endif ()

    if (cppdummy_ENABLE_COVERAGE)
        include(cmake/Tests.cmake)
        cppdummy_enable_coverage(cppdummy_options)
    endif ()

    if (cppdummy_WARNINGS_AS_ERRORS)
        check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
        if (LINKER_FATAL_WARNINGS)
            target_link_options(cppdummy_options INTERFACE -Wl,--fatal-warnings)
        endif ()
    endif ()

    if (cppdummy_ENABLE_HARDENING AND NOT cppdummy_ENABLE_GLOBAL_HARDENING)
        include(cmake/Hardening.cmake)

        if (NOT SUPPORTS_UBSAN
                OR cppdummy_ENABLE_SANITIZER_UNDEFINED
                OR cppdummy_ENABLE_SANITIZER_ADDRESS
                OR cppdummy_ENABLE_SANITIZER_THREAD
                OR cppdummy_ENABLE_SANITIZER_LEAK)
            set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
        else ()
            set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
        endif ()

        cppdummy_enable_hardening(cppdummy_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
    endif ()
endmacro()