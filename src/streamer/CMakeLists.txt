find_package(GStreamer REQUIRED)

add_executable(streamer
    main.cpp
)

target_link_libraries(streamer
    PRIVATE
    cppdummy::cppdummy_options
    cppdummy::cppdummy_warnings
)

target_link_system_libraries(streamer
    PRIVATE
    GStreamer::Core
    GStreamer::Video
)
