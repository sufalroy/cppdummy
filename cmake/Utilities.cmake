function(find_substring_by_prefix output prefix input)
    # Find the prefix
    string(FIND "${input}" "${prefix}" prefix_index)
    if (prefix_index EQUAL -1)
        message(WARNING "Could not find '${prefix}' in input string")
        set("${output}" "" PARENT_SCOPE)
        return()
    endif ()

    # Calculate start index
    string(LENGTH "${prefix}" prefix_length)
    math(EXPR start_index "${prefix_index} + ${prefix_length}")

    # Extract substring
    string(SUBSTRING "${input}" "${start_index}" -1 _output)
    set("${output}" "${_output}" PARENT_SCOPE)
endfunction()

function(set_env_from_string env_string)
    if (NOT env_string)
        message(WARNING "Empty environment string provided")
        return()
    endif ()

    # Replace semicolons in paths with placeholder
    string(REGEX REPLACE ";" "__SEMICOLON__" env_string_escaped "${env_string}")

    # Split on newlines (handle both \r\n and \n)
    string(REGEX REPLACE "\r?\n" ";" env_list "${env_string_escaped}")

    set(vars_set 0)
    foreach (env_var ${env_list})
        # Skip empty lines
        string(STRIP "${env_var}" env_var_trimmed)
        if (NOT env_var_trimmed)
            continue()
        endif ()

        # Find first equals sign (variable names can't contain =, but values can)
        string(FIND "${env_var_trimmed}" "=" equals_pos)
        if (equals_pos GREATER 0)
            # Extract name and value
            string(SUBSTRING "${env_var_trimmed}" 0 ${equals_pos} env_name)
            math(EXPR value_start "${equals_pos} + 1")
            string(SUBSTRING "${env_var_trimmed}" ${value_start} -1 env_value)

            # Restore semicolons in value
            string(REGEX REPLACE "__SEMICOLON__" ";" env_value "${env_value}")

            # Set the environment variable
            set(ENV{${env_name}} "${env_value}")
            math(EXPR vars_set "${vars_set} + 1")

            # Update CMAKE_PROGRAM_PATH for PATH variable
            if (env_name STREQUAL "PATH")
                # Convert Windows paths to CMake format and append to CMAKE_PROGRAM_PATH
                string(REPLACE ";" "\\;" path_list "${env_value}")
                file(TO_CMAKE_PATH "${path_list}" cmake_path_list)
                string(REPLACE "\\;" ";" cmake_path_list "${cmake_path_list}")
                list(APPEND CMAKE_PROGRAM_PATH ${cmake_path_list})
                set(CMAKE_PROGRAM_PATH "${CMAKE_PROGRAM_PATH}" PARENT_SCOPE)
            endif ()
        endif ()
    endforeach ()

    message(STATUS "Set ${vars_set} environment variables from vcvarsall output")
endfunction()

function(get_all_targets var)
    set(targets)
    get_all_targets_recursive(targets ${CMAKE_CURRENT_SOURCE_DIR})
    # Remove duplicates and sort
    list(REMOVE_DUPLICATES targets)
    list(SORT targets)
    set(${var} ${targets} PARENT_SCOPE)
endfunction()

function(get_all_installable_targets var)
    set(targets)
    get_all_targets(targets)
    set(installable_targets)

    foreach (_target ${targets})
        # Check if target exists and get its type
        if (TARGET ${_target})
            get_target_property(_target_type ${_target} TYPE)
            # Include libraries and executables, exclude interface and utility targets
            if (_target_type MATCHES "^(STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|EXECUTABLE)$")
                list(APPEND installable_targets ${_target})
            endif ()
        endif ()
    endforeach ()

    set(${var} ${installable_targets} PARENT_SCOPE)
endfunction()

macro(get_all_targets_recursive targets dir)
    # Verify directory exists
    if (NOT EXISTS "${dir}")
        message(WARNING "Directory does not exist: ${dir}")
        return()
    endif ()

    get_property(subdirectories DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
    foreach (subdir ${subdirectories})
        get_all_targets_recursive(${targets} ${subdir})
    endforeach ()

    get_property(current_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    if (current_targets)
        list(APPEND ${targets} ${current_targets})
    endif ()
endmacro()

function(is_verbose var)
    set(is_verbose_result OFF)

    # Check CMAKE_MESSAGE_LOG_LEVEL
    if (CMAKE_MESSAGE_LOG_LEVEL)
        if (CMAKE_MESSAGE_LOG_LEVEL MATCHES "^(VERBOSE|DEBUG|TRACE)$")
            set(is_verbose_result ON)
        endif ()
    endif ()

    # Also check for --verbose flag or VERBOSE environment variable
    if (NOT is_verbose_result)
        if (DEFINED ENV{VERBOSE} AND NOT "$ENV{VERBOSE}" STREQUAL "")
            set(is_verbose_result ON)
        endif ()
    endif ()

    set(${var} ${is_verbose_result} PARENT_SCOPE)
endfunction()
