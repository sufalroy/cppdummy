include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)

if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_GSTREAMER QUIET gstreamer-1.0)
    pkg_check_modules(PC_GSTREAMER_VIDEO QUIET gstreamer-video-1.0)
    
    if(PC_GSTREAMER_FOUND)
        set(GSTREAMER_VERSION ${PC_GSTREAMER_VERSION})
    endif()
endif()

if(WIN32)
    set(GSTREAMER_SEARCH_PATHS
        $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64}
        "C:/Program Files/gstreamer/1.0/msvc_x86_64"
        "C:/gstreamer/1.0/msvc_x86_64"
    )
    
    find_path(GSTREAMER_ROOT_DIR
        NAMES include/gstreamer-1.0/gst/gst.h
        PATHS ${GSTREAMER_SEARCH_PATHS}
        DOC "GStreamer installation root directory"
    )
    
    if(GSTREAMER_ROOT_DIR)
        set(GSTREAMER_INCLUDE_DIRS
            "${GSTREAMER_ROOT_DIR}/include/gstreamer-1.0"
            "${GSTREAMER_ROOT_DIR}/include/glib-2.0"
            "${GSTREAMER_ROOT_DIR}/lib/glib-2.0/include"
        )
        
        find_library(GSTREAMER_LIBRARY
            NAMES gstreamer-1.0
            PATHS "${GSTREAMER_ROOT_DIR}/lib"
            NO_DEFAULT_PATH
        )
        
        find_library(GSTREAMER_VIDEO_LIBRARY
            NAMES gstvideo-1.0
            PATHS "${GSTREAMER_ROOT_DIR}/lib"
            NO_DEFAULT_PATH
        )
        
        find_library(GLIB_LIBRARY
            NAMES glib-2.0
            PATHS "${GSTREAMER_ROOT_DIR}/lib"
            NO_DEFAULT_PATH
        )
        
        find_library(GOBJECT_LIBRARY
            NAMES gobject-2.0
            PATHS "${GSTREAMER_ROOT_DIR}/lib"
            NO_DEFAULT_PATH
        )
    endif()
else()
    if(PC_GSTREAMER_FOUND)
        set(GSTREAMER_INCLUDE_DIRS ${PC_GSTREAMER_INCLUDE_DIRS})
        set(GSTREAMER_LIBRARIES ${PC_GSTREAMER_LIBRARIES})
        set(GSTREAMER_VIDEO_LIBRARIES ${PC_GSTREAMER_VIDEO_LIBRARIES})
    else()
        find_path(GSTREAMER_INCLUDE_DIR
            NAMES gst/gst.h
            PATH_SUFFIXES gstreamer-1.0
        )
        
        find_library(GSTREAMER_LIBRARY
            NAMES gstreamer-1.0
        )
        
        find_library(GSTREAMER_VIDEO_LIBRARY
            NAMES gstvideo-1.0
        )
        
        if(GSTREAMER_INCLUDE_DIR)
            set(GSTREAMER_INCLUDE_DIRS ${GSTREAMER_INCLUDE_DIR})
        endif()
    endif()
endif()

find_package_handle_standard_args(GStreamer
    REQUIRED_VARS GSTREAMER_LIBRARY GSTREAMER_INCLUDE_DIRS
    VERSION_VAR GSTREAMER_VERSION
)

if(GSTREAMER_FOUND)
    if(NOT TARGET GStreamer::Core)
        add_library(GStreamer::Core UNKNOWN IMPORTED)
        set_target_properties(GStreamer::Core PROPERTIES
            IMPORTED_LOCATION "${GSTREAMER_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GSTREAMER_INCLUDE_DIRS}"
        )
        
        if(WIN32 AND GLIB_LIBRARY AND GOBJECT_LIBRARY)
            set_target_properties(GStreamer::Core PROPERTIES
                INTERFACE_LINK_LIBRARIES "${GLIB_LIBRARY};${GOBJECT_LIBRARY}"
            )
        endif()
        
        if(PC_GSTREAMER_CFLAGS_OTHER)
            set_target_properties(GStreamer::Core PROPERTIES
                INTERFACE_COMPILE_OPTIONS "${PC_GSTREAMER_CFLAGS_OTHER}"
            )
        endif()
    endif()
    
    if(GSTREAMER_VIDEO_LIBRARY AND NOT TARGET GStreamer::Video)
        add_library(GStreamer::Video UNKNOWN IMPORTED)
        set_target_properties(GStreamer::Video PROPERTIES
            IMPORTED_LOCATION "${GSTREAMER_VIDEO_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GSTREAMER_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "GStreamer::Core"
        )
        
        if(PC_GSTREAMER_VIDEO_CFLAGS_OTHER)
            set_target_properties(GStreamer::Video PROPERTIES
                INTERFACE_COMPILE_OPTIONS "${PC_GSTREAMER_VIDEO_CFLAGS_OTHER}"
            )
        endif()
    endif()
endif()

mark_as_advanced(
    GSTREAMER_ROOT_DIR
    GSTREAMER_INCLUDE_DIR
    GSTREAMER_LIBRARY
    GSTREAMER_VIDEO_LIBRARY
    GLIB_LIBRARY
    GOBJECT_LIBRARY
)