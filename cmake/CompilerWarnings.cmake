# Based on: https://github.com/lefticus/cppbestpractices/blob/master/02-Use_the_Tools_Available.md

function(
        cppdummy_set_project_warnings
        project_name
        WARNINGS_AS_ERRORS
        MSVC_WARNINGS
        CLANG_WARNINGS
        GCC_WARNINGS
        CUDA_WARNINGS)

    # Validate that target exists
    if (NOT TARGET ${project_name})
        message(FATAL_ERROR "Target '${project_name}' does not exist")
    endif ()

    # MSVC Warning Configuration
    if ("${MSVC_WARNINGS}" STREQUAL "")
        set(MSVC_WARNINGS
                /W4 # Baseline reasonable warnings
                /w14242 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
                /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
                /w14263 # 'function': member function does not override any base class virtual member function
                /w14265 # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not
                # be destructed correctly
                /w14287 # 'operator': unsigned/negative constant mismatch
                /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside
                # the for-loop scope
                /w14296 # 'operator': expression is always 'boolean_value'
                /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
                /w14545 # expression before comma evaluates to a function which is missing an argument list
                /w14546 # function call before comma missing argument list
                /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
                /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
                /w14555 # expression has no effect; expected expression with side- effect
                /w14619 # pragma warning: there is no warning number 'number'
                /w14640 # Enable warning on thread un-safe static member initialization
                /w14826 # Conversion from 'type1' to 'type2' is sign-extended. This may cause unexpected runtime behavior.
                /w14905 # wide string literal cast to 'LPSTR'
                /w14906 # string literal cast to 'LPWSTR'
                /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
                /permissive- # standards conformance mode for MSVC compiler.
        )
    endif ()

    # Clang Warning Configuration
    if ("${CLANG_WARNINGS}" STREQUAL "")
        set(CLANG_WARNINGS
                -Wall
                -Wextra # reasonable and standard
                -Wshadow # warn the user if a variable declaration shadows one from a parent context
                -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps
                # catch hard to track down memory errors
                -Wold-style-cast # warn for c-style casts
                -Wcast-align # warn for potential performance problem casts
                -Wunused # warn on anything being unused
                -Woverloaded-virtual # warn if you overload (not override) a virtual function
                -Wpedantic # warn if non-standard C++ is used
                -Wconversion # warn on type conversions that may lose data
                -Wsign-conversion # warn on sign conversions
                -Wnull-dereference # warn if a null dereference is detected
                -Wdouble-promotion # warn if float is implicit promoted to double
                -Wformat=2 # warn on security issues around functions that format output (ie printf)
                -Wimplicit-fallthrough # warn on statements that fallthrough without an explicit annotation
        )
    endif ()

    # GCC Warning Configuration (inherits Clang warnings + GCC-specific ones)
    if ("${GCC_WARNINGS}" STREQUAL "")
        set(GCC_WARNINGS
                ${CLANG_WARNINGS}
                -Wmisleading-indentation # warn if indentation implies blocks where blocks do not exist
                -Wduplicated-cond # warn if if / else chain has duplicated conditions
                -Wduplicated-branches # warn if if / else branches have duplicated code
                -Wlogical-op # warn about logical operations being used where bitwise were probably wanted
                -Wuseless-cast # warn if you perform a cast to the same type
                -Wsuggest-override # warn if an overridden member function is not marked 'override' or 'final'
        )
    endif ()

    # CUDA Warning Configuration
    if ("${CUDA_WARNINGS}" STREQUAL "")
        set(CUDA_WARNINGS
                -Wall
                -Wextra
                -Wunused
                -Wconversion
                -Wshadow
                # TODO: Add more CUDA-specific warnings as needed
        )
    endif ()

    # Apply warnings-as-errors if requested
    if (WARNINGS_AS_ERRORS)
        message(STATUS "Warnings are treated as errors for target '${project_name}'")
        list(APPEND CLANG_WARNINGS -Werror)
        list(APPEND GCC_WARNINGS -Werror)
        list(APPEND MSVC_WARNINGS /WX)
        list(APPEND CUDA_WARNINGS -Werror)
    endif ()

    # Determine compiler-specific warnings
    if (MSVC)
        set(PROJECT_WARNINGS_CXX ${MSVC_WARNINGS})
        message(DEBUG "Using MSVC warnings for target '${project_name}'")
    elseif (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(PROJECT_WARNINGS_CXX ${CLANG_WARNINGS})
        message(DEBUG "Using Clang warnings for target '${project_name}'")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(PROJECT_WARNINGS_CXX ${GCC_WARNINGS})
        message(DEBUG "Using GCC warnings for target '${project_name}'")
    else ()
        message(AUTHOR_WARNING "No compiler warnings set for CXX compiler: '${CMAKE_CXX_COMPILER_ID}' on target '${project_name}'")
        # Set empty warnings to avoid undefined variable errors
        set(PROJECT_WARNINGS_CXX "")
    endif ()

    # Use the same warning flags for C
    # Note: Some C++ specific flags might not be applicable to C
    set(PROJECT_WARNINGS_C "${PROJECT_WARNINGS_CXX}")

    # Set CUDA warnings
    set(PROJECT_WARNINGS_CUDA "${CUDA_WARNINGS}")

    # Apply warnings to target using modern CMake generator expressions
    target_compile_options(
            ${project_name}
            INTERFACE
            # C++ warnings
            $<$<COMPILE_LANGUAGE:CXX>:${PROJECT_WARNINGS_CXX}>
            # C warnings
            $<$<COMPILE_LANGUAGE:C>:${PROJECT_WARNINGS_C}>
            # CUDA warnings
            $<$<COMPILE_LANGUAGE:CUDA>:${PROJECT_WARNINGS_CUDA}>
    )

    # Optional: Print applied warnings for debugging (only in verbose mode)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(VERBOSE "Applied warnings to target '${project_name}':")
        message(VERBOSE "  CXX: ${PROJECT_WARNINGS_CXX}")
        message(VERBOSE "  C:   ${PROJECT_WARNINGS_C}")
        message(VERBOSE "  CUDA: ${PROJECT_WARNINGS_CUDA}")
    endif ()
endfunction()