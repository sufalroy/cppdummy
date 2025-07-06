include(cmake/CPM.cmake)

set(CPM_USE_LOCAL_PACKAGES ON)
set(CPM_LOCAL_PACKAGES_ONLY OFF)

function(cppdummy_setup_dependencies)

    if (NOT TARGET fmt::fmt)
        CPMAddPackage(
                NAME fmt
                GITHUB_REPOSITORY fmtlib/fmt
                GIT_TAG 11.1.4
                OPTIONS
                "FMT_INSTALL ON"
                "FMT_TEST OFF"
                "FMT_DOC OFF"
        )
    endif ()

    if (NOT TARGET spdlog::spdlog)
        CPMAddPackage(
                NAME spdlog
                GITHUB_REPOSITORY gabime/spdlog
                VERSION 1.15.2
                OPTIONS
                "SPDLOG_FMT_EXTERNAL ON"
                "SPDLOG_INSTALL ON"
                "SPDLOG_BUILD_TESTS OFF"
                "SPDLOG_BUILD_EXAMPLE OFF"
        )
    endif ()

    if (NOT TARGET glm::glm)
        CPMAddPackage(
                NAME glm
                GITHUB_REPOSITORY g-truc/glm
                GIT_TAG 1.0.1
                OPTIONS
                "GLM_BUILD_TESTS OFF"
        )
    endif ()

    if (NOT TARGET glfw)
        CPMAddPackage(
                NAME glfw
                GITHUB_REPOSITORY glfw/glfw
                GIT_TAG 3.4
                OPTIONS
                "GLFW_BUILD_EXAMPLES OFF"
                "GLFW_BUILD_TESTS OFF"
                "GLFW_BUILD_DOCS OFF"
                "GLFW_INSTALL ON"
                "GLFW_VULKAN_STATIC OFF"
        )
    endif ()
endfunction()