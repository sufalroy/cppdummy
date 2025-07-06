find_package(Vulkan REQUIRED)

if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found. Please install LunarG Vulkan SDK and set VULKAN_SDK environment variable.")
endif ()

add_executable(intro
        main.cpp
)

target_link_libraries(intro
        PRIVATE
        cppdummy::cppdummy_options
        cppdummy::cppdummy_warnings
)

target_link_libraries(intro
        PRIVATE
        fmt::fmt
        spdlog::spdlog
        glm::glm
        glfw
        Vulkan::Vulkan
)

if (WIN32)
    target_compile_definitions(intro PRIVATE
            NOMINMAX
            WIN32_LEAN_AND_MEAN
            _CRT_SECURE_NO_WARNINGS
    )

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_target_properties(intro PROPERTIES
                WIN32_EXECUTABLE FALSE
        )
    else ()
        set_target_properties(intro PROPERTIES
                WIN32_EXECUTABLE TRUE
        )
    endif ()
endif ()