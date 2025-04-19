include(FetchContent)
mark_as_advanced(FETCHCONTENT_QUIET)
mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)

function(fetch_dependency NAME)
  cmake_parse_arguments(DEP
    "QUIET"
    "GIT_REPOSITORY;GIT_TAG;URL;URL_HASH;SOURCE_DIR"
    "CMAKE_ARGS;PATCH_COMMAND"
    ${ARGN}
  )

  string(TOLOWER ${NAME} NAME_LOWER)

  if(DEP_QUIET)
    set(QUIET_ARG QUIET)
  else()
    set(QUIET_ARG "")
  endif()

  if(NOT DEP_GIT_TAG)
    set(DEP_GIT_TAG "main")
  endif()

  if(DEP_GIT_REPOSITORY)
    message(STATUS "Configuring dependency: ${NAME} from ${DEP_GIT_REPOSITORY}")
    FetchContent_Declare(
      ${NAME_LOWER}
      GIT_REPOSITORY ${DEP_GIT_REPOSITORY}
      GIT_TAG ${DEP_GIT_TAG}
      ${QUIET_ARG}
      ${DEP_UNPARSED_ARGUMENTS}
    )
  elseif(DEP_URL)
    message(STATUS "Configuring dependency: ${NAME} from ${DEP_URL}")
    FetchContent_Declare(
      ${NAME_LOWER}
      URL ${DEP_URL}
      URL_HASH ${DEP_URL_HASH}
      ${QUIET_ARG}
      ${DEP_UNPARSED_ARGUMENTS}
    )
  elseif(DEP_SOURCE_DIR)
    message(STATUS "Configuring dependency: ${NAME} from local path ${DEP_SOURCE_DIR}")
    FetchContent_Declare(
      ${NAME_LOWER}
      SOURCE_DIR ${DEP_SOURCE_DIR}
      ${QUIET_ARG}
      ${DEP_UNPARSED_ARGUMENTS}
    )
  else()
    message(FATAL_ERROR "No source specified for dependency ${NAME}")
  endif()

  FetchContent_GetProperties(${NAME_LOWER})
  if(NOT ${NAME_LOWER}_POPULATED)
    if(DEP_CMAKE_ARGS)
      set(CMAKE_ARGS ${DEP_CMAKE_ARGS})
    else()
      set(CMAKE_ARGS "")
    endif()

    if(DEP_PATCH_COMMAND)
      set(PATCH_COMMAND ${DEP_PATCH_COMMAND})
    else()
      set(PATCH_COMMAND "")
    endif()

    FetchContent_Populate(${NAME_LOWER})
    
    if(PATCH_COMMAND)
      execute_process(
        COMMAND ${PATCH_COMMAND}
        WORKING_DIRECTORY ${${NAME_LOWER}_SOURCE_DIR}
        RESULT_VARIABLE PATCH_RESULT
      )
      if(NOT PATCH_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to apply patch to ${NAME}")
      endif()
    endif()

    if(EXISTS "${${NAME_LOWER}_SOURCE_DIR}/CMakeLists.txt")
      if(CMAKE_ARGS)
        set(CMAKE_ARGS_BACKUP ${CMAKE_ARGS})
        foreach(ARG ${CMAKE_ARGS})
          if(ARG MATCHES "^([A-Za-z0-9_-]+)=(.*)$")
            set(${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")
          endif()
        endforeach()
      endif()

      add_subdirectory(${${NAME_LOWER}_SOURCE_DIR} ${${NAME_LOWER}_BINARY_DIR} EXCLUDE_FROM_ALL)

      if(CMAKE_ARGS)
        foreach(ARG ${CMAKE_ARGS_BACKUP})
          if(ARG MATCHES "^([A-Za-z0-9_-]+)=(.*)$")
            unset(${CMAKE_MATCH_1})
          endif()
        endforeach()
      endif()
    endif()
  endif()

  set(${NAME_LOWER}_SOURCE_DIR ${${NAME_LOWER}_SOURCE_DIR} PARENT_SCOPE)
  set(${NAME_LOWER}_BINARY_DIR ${${NAME_LOWER}_BINARY_DIR} PARENT_SCOPE)
  set(${NAME_LOWER}_POPULATED TRUE PARENT_SCOPE)
endfunction()

function(fetch_header_only_lib NAME)
  cmake_parse_arguments(LIB
    "QUIET;INTERFACE"
    "GIT_REPOSITORY;GIT_TAG;URL;URL_HASH;SOURCE_DIR;INCLUDE_DIR;TARGET_NAME"
    "PUBLIC_HEADERS;PRIVATE_HEADERS;DEFINITIONS;COMPILE_OPTIONS;INCLUDE_DIRS;LINK_LIBRARIES"
    ${ARGN}
  )

  if(NOT LIB_TARGET_NAME)
    set(LIB_TARGET_NAME ${NAME})
  endif()

  if(NOT LIB_INCLUDE_DIR)
    set(LIB_INCLUDE_DIR "include")
  endif()

  if(LIB_QUIET)
    fetch_dependency(${NAME} QUIET 
      GIT_REPOSITORY ${LIB_GIT_REPOSITORY}
      GIT_TAG ${LIB_GIT_TAG}
      URL ${LIB_URL}
      URL_HASH ${LIB_URL_HASH}
      SOURCE_DIR ${LIB_SOURCE_DIR}
      ${LIB_UNPARSED_ARGUMENTS}
    )
  else()
    fetch_dependency(${NAME}
      GIT_REPOSITORY ${LIB_GIT_REPOSITORY}
      GIT_TAG ${LIB_GIT_TAG}
      URL ${LIB_URL}
      URL_HASH ${LIB_URL_HASH}
      SOURCE_DIR ${LIB_SOURCE_DIR}
      ${LIB_UNPARSED_ARGUMENTS}
    )
  endif()

  string(TOLOWER ${NAME} NAME_LOWER)
  
  if(NOT TARGET ${LIB_TARGET_NAME})
    if(LIB_INTERFACE)
      add_library(${LIB_TARGET_NAME} INTERFACE)
    else()
      add_library(${LIB_TARGET_NAME} STATIC)
      set_target_properties(${LIB_TARGET_NAME} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        VISIBILITY_INLINES_HIDDEN ON
        CXX_VISIBILITY_PRESET hidden
      )
    endif()
    
    string(CONCAT INCLUDE_PATH ${${NAME_LOWER}_SOURCE_DIR} "/" ${LIB_INCLUDE_DIR})
    if(EXISTS ${INCLUDE_PATH})
      if(LIB_INTERFACE)
        target_include_directories(${LIB_TARGET_NAME} INTERFACE ${INCLUDE_PATH})
      else()
        target_include_directories(${LIB_TARGET_NAME} PUBLIC ${INCLUDE_PATH})
      endif()
    else()
      if(LIB_INTERFACE)
        target_include_directories(${LIB_TARGET_NAME} INTERFACE ${${NAME_LOWER}_SOURCE_DIR})
      else()
        target_include_directories(${LIB_TARGET_NAME} PUBLIC ${${NAME_LOWER}_SOURCE_DIR})
      endif()
    endif()

    if(LIB_INCLUDE_DIRS)
      foreach(DIR ${LIB_INCLUDE_DIRS})
        if(LIB_INTERFACE)
          target_include_directories(${LIB_TARGET_NAME} INTERFACE ${DIR})
        else()
          target_include_directories(${LIB_TARGET_NAME} PUBLIC ${DIR})
        endif()
      endforeach()
    endif()

    if(LIB_DEFINITIONS)
      if(LIB_INTERFACE)
        target_compile_definitions(${LIB_TARGET_NAME} INTERFACE ${LIB_DEFINITIONS})
      else()
        target_compile_definitions(${LIB_TARGET_NAME} PUBLIC ${LIB_DEFINITIONS})
      endif()
    endif()

    if(LIB_COMPILE_OPTIONS)
      if(LIB_INTERFACE)
        target_compile_options(${LIB_TARGET_NAME} INTERFACE ${LIB_COMPILE_OPTIONS})
      else()
        target_compile_options(${LIB_TARGET_NAME} PUBLIC ${LIB_COMPILE_OPTIONS})
      endif()
    endif()

    if(LIB_LINK_LIBRARIES)
      if(LIB_INTERFACE)
        target_link_libraries(${LIB_TARGET_NAME} INTERFACE ${LIB_LINK_LIBRARIES})
      else()
        target_link_libraries(${LIB_TARGET_NAME} PUBLIC ${LIB_LINK_LIBRARIES})
      endif()
    endif()

    if(NOT LIB_INTERFACE AND LIB_PUBLIC_HEADERS)
      target_sources(${LIB_TARGET_NAME} PUBLIC ${LIB_PUBLIC_HEADERS})
    endif()

    if(NOT LIB_INTERFACE AND LIB_PRIVATE_HEADERS)
      target_sources(${LIB_TARGET_NAME} PRIVATE ${LIB_PRIVATE_HEADERS})
    endif()
  endif()
endfunction()