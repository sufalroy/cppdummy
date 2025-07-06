include(CheckCXXCompilerFlag)

macro(
        cppdummy_enable_hardening
        target
        global
        ubsan_minimal_runtime)

    message(STATUS "** Enabling Hardening (Target ${target}) **")

    # Initialize variables properly
    set(NEW_COMPILE_OPTIONS "")
    set(NEW_LINK_OPTIONS "")
    set(NEW_CXX_DEFINITIONS "")

    if (MSVC)
        # MSVC-specific hardening flags
        list(APPEND NEW_COMPILE_OPTIONS /sdl /guard:cf)
        list(APPEND NEW_LINK_OPTIONS /DYNAMICBASE /NXCOMPAT /CETCOMPAT)
        message(STATUS "*** MSVC flags: /sdl /guard:cf /DYNAMICBASE /NXCOMPAT /CETCOMPAT")

    elseif (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # Enable GLIBC++ assertions for additional bounds checking
        list(APPEND NEW_CXX_DEFINITIONS _GLIBCXX_ASSERTIONS)
        message(STATUS "*** GLIBC++ Assertions (vector[], string[], ...) enabled")

        # Enable FORTIFY_SOURCE for buffer overflow protection (not in Debug mode)
        if (NOT CMAKE_BUILD_TYPE MATCHES "Debug")
            list(APPEND NEW_CXX_DEFINITIONS _FORTIFY_SOURCE=3)
            message(STATUS "*** g++/clang _FORTIFY_SOURCE=3 enabled")
        else ()
            message(STATUS "*** _FORTIFY_SOURCE disabled in Debug mode")
        endif ()

        # Stack protector for stack-based buffer overflow protection
        check_cxx_compiler_flag(-fstack-protector-strong STACK_PROTECTOR)
        if (STACK_PROTECTOR)
            list(APPEND NEW_COMPILE_OPTIONS -fstack-protector-strong)
            message(STATUS "*** g++/clang -fstack-protector-strong enabled")
        else ()
            message(STATUS "*** g++/clang -fstack-protector-strong NOT enabled (not supported)")
        endif ()

        # Control Flow Integrity protection
        check_cxx_compiler_flag(-fcf-protection CF_PROTECTION)
        if (CF_PROTECTION)
            list(APPEND NEW_COMPILE_OPTIONS -fcf-protection)
            message(STATUS "*** g++/clang -fcf-protection enabled")
        else ()
            message(STATUS "*** g++/clang -fcf-protection NOT enabled (not supported)")
        endif ()

        # Stack clash protection (primarily for GCC and Linux)
        check_cxx_compiler_flag(-fstack-clash-protection CLASH_PROTECTION)
        if (CLASH_PROTECTION)
            if (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                list(APPEND NEW_COMPILE_OPTIONS -fstack-clash-protection)
                message(STATUS "*** g++/clang -fstack-clash-protection enabled")
            else ()
                message(STATUS "*** g++/clang -fstack-clash-protection NOT enabled (clang on non-Linux)")
            endif ()
        else ()
            message(STATUS "*** g++/clang -fstack-clash-protection NOT enabled (not supported)")
        endif ()

        # Position Independent Executable (PIE) - uncommented and improved
        check_cxx_compiler_flag(-fpie PIE_SUPPORTED)
        if (PIE_SUPPORTED)
            list(APPEND NEW_COMPILE_OPTIONS -fpie)
            list(APPEND NEW_LINK_OPTIONS -pie)
            message(STATUS "*** g++/clang PIE mode enabled")
        else ()
            message(STATUS "*** g++/clang PIE mode NOT enabled (not supported)")
        endif ()

        # Additional hardening flags
        check_cxx_compiler_flag(-Wformat-security FORMAT_SECURITY)
        if (FORMAT_SECURITY)
            list(APPEND NEW_COMPILE_OPTIONS -Wformat -Wformat-security)
            message(STATUS "*** Format security warnings enabled")
        endif ()

        # Enable additional security warnings
        check_cxx_compiler_flag(-Warray-bounds ARRAY_BOUNDS)
        if (ARRAY_BOUNDS)
            list(APPEND NEW_COMPILE_OPTIONS -Warray-bounds)
        endif ()
    endif ()

    # UBSan (Undefined Behavior Sanitizer) support
    if (${ubsan_minimal_runtime})
        check_cxx_compiler_flag("-fsanitize=undefined -fno-sanitize-recover=undefined -fsanitize-minimal-runtime"
                MINIMAL_RUNTIME)
        if (MINIMAL_RUNTIME)
            list(APPEND NEW_COMPILE_OPTIONS -fsanitize=undefined -fsanitize-minimal-runtime)
            list(APPEND NEW_LINK_OPTIONS -fsanitize=undefined -fsanitize-minimal-runtime)

            if (NOT ${global})
                list(APPEND NEW_COMPILE_OPTIONS -fno-sanitize-recover=undefined)
                list(APPEND NEW_LINK_OPTIONS -fno-sanitize-recover=undefined)
            else ()
                message(STATUS "** not enabling -fno-sanitize-recover=undefined for global consumption")
            endif ()

            message(STATUS "*** ubsan minimal runtime enabled")
        else ()
            message(STATUS "*** ubsan minimal runtime NOT enabled (not supported)")
        endif ()
    else ()
        message(STATUS "*** ubsan minimal runtime NOT enabled (not requested)")
    endif ()

    # Convert lists to strings for display
    string(REPLACE ";" " " COMPILE_OPTIONS_STR "${NEW_COMPILE_OPTIONS}")
    string(REPLACE ";" " " LINK_OPTIONS_STR "${NEW_LINK_OPTIONS}")
    string(REPLACE ";" " -D" CXX_DEFINITIONS_STR "${NEW_CXX_DEFINITIONS}")
    if (NEW_CXX_DEFINITIONS)
        set(CXX_DEFINITIONS_STR "-D${CXX_DEFINITIONS_STR}")
    endif ()

    message(STATUS "** Hardening Compiler Flags: ${COMPILE_OPTIONS_STR}")
    message(STATUS "** Hardening Linker Flags: ${LINK_OPTIONS_STR}")
    message(STATUS "** Hardening Compiler Defines: ${CXX_DEFINITIONS_STR}")

    # Apply the hardening flags
    if (${global})
        message(STATUS "** Setting hardening options globally for all dependencies")
        # Note: Global application affects all targets - use with caution
        add_compile_options(${NEW_COMPILE_OPTIONS})
        add_link_options(${NEW_LINK_OPTIONS})
        add_compile_definitions(${NEW_CXX_DEFINITIONS})
    else ()
        # Apply to specific target only
        if (TARGET ${target})
            target_compile_options(${target} INTERFACE ${NEW_COMPILE_OPTIONS})
            target_link_options(${target} INTERFACE ${NEW_LINK_OPTIONS})
            target_compile_definitions(${target} INTERFACE ${NEW_CXX_DEFINITIONS})
        else ()
            message(WARNING "Target '${target}' does not exist. Hardening flags not applied.")
        endif ()
    endif ()

    message(STATUS "** Hardening configuration complete **")
endmacro()
