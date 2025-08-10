cmake_minimum_required(VERSION 3.22)

include(cmake/PreventInSourceBuilds.cmake)

project(cppdummy
        VERSION 0.0.1
        DESCRIPTION "A modern C++ project for dummies"
        HOMEPAGE_URL "https://github.com/sufalroy/cppdummy.git"
        LANGUAGES CXX C
)

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif ()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(ProjectOptions.cmake)
cppdummy_setup_options()
cppdummy_global_options()

include(Dependencies.cmake)
cppdummy_setup_dependencies()

cppdummy_local_options()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

target_compile_features(cppdummy_options INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

add_library(cppdummy::cppdummy_options ALIAS cppdummy_options)
add_library(cppdummy::cppdummy_warnings ALIAS cppdummy_warnings)

add_subdirectory(src)

if (NOT PROJECT_IS_TOP_LEVEL)
    return()
endif ()

if(MSVC)
    get_all_installable_targets(all_targets)
    message("all_targets=${all_targets}")
    set_target_properties(${all_targets} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=$(VC_ExecutablePath_x64);%PATH%")
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT intro)

if (CMAKE_SKIP_INSTALL_RULES)
    return()
endif ()

include(cmake/PackageProject.cmake)

cppdummy_package_project(
        TARGETS
        intro
        cppdummy_options
        cppdummy_warnings
)

set(CPACK_PACKAGE_FILE_NAME
        "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${GIT_SHORT_SHA}-${CMAKE_SYSTEM_NAME}-${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
)

include(CPack)