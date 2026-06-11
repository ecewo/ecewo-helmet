cmake_minimum_required(VERSION 3.14)
project(ecewo_helmet VERSION 0.2.0 LANGUAGES C)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
endif()

option(ECEWO_HELMET_BUILD_SHARED "Build ecewo-helmet as a shared library" OFF)
option(ECEWO_HELMET_BUILD_TESTS "Build ecewo-helmet tests" ${PROJECT_IS_TOP_LEVEL})

if(NOT TARGET ecewo::ecewo)
  if(DEFINED ECEWO_BUILD_SHARED)
    set(ECEWO_HELMET_BUILD_SHARED "${ECEWO_BUILD_SHARED}" CACHE BOOL "" FORCE)
  endif()

  set(ECEWO_BUILD_SHARED "${ECEWO_HELMET_BUILD_SHARED}" CACHE BOOL "" FORCE)

  include(FetchContent)
  FetchContent_Declare(
    ecewo
    GIT_REPOSITORY https://github.com/ecewo/ecewo.git
    GIT_TAG v4.0.0
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(ecewo)
endif()

if(ECEWO_HELMET_BUILD_SHARED)
  add_library(ecewo_helmet SHARED src/ecewo-helmet.c)
  message(STATUS "Building ecewo_helmet as SHARED library")
else()
  add_library(ecewo_helmet STATIC src/ecewo-helmet.c)
  message(STATUS "Building ecewo_helmet as STATIC library")
endif()

add_library(ecewo::helmet ALIAS ecewo_helmet)

include(GenerateExportHeader)
generate_export_header(ecewo_helmet
  BASE_NAME ECEWO_HELMET
  EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/include/ecewo-helmet-export.h"
)

target_include_directories(ecewo_helmet PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(ecewo_helmet PUBLIC ecewo::ecewo)

set_target_properties(ecewo_helmet PROPERTIES
  OUTPUT_NAME ecewo_helmet
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  EXPORT_NAME helmet
  C_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  POSITION_INDEPENDENT_CODE ON
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(ecewo_helmet PRIVATE
    -Wall -Wextra -Wstrict-prototypes -Wincompatible-pointer-types
  )
endif()

if(ECEWO_HELMET_BUILD_TESTS AND TARGET ecewo::ecewo)
  ecewo_add(mock@dev)
  enable_testing()

  function(add_helmet_test test_name)
    set(target_name ecewo_helmet_${test_name}_test)

    add_executable(${target_name} tests/test-${test_name}.c)

    target_include_directories(${target_name} PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
      ${CMAKE_CURRENT_BINARY_DIR}/include
    )

    target_link_libraries(${target_name} PRIVATE
      ecewo::ecewo
      ecewo::helmet
      ecewo::mock
    )

    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(${target_name} PRIVATE
        -Wall -Wextra -Werror
        -Wstrict-prototypes
        -Wincompatible-pointer-types
        -Wno-unused-parameter
      )
    endif()

    add_test(NAME ECEWO_HELMET_${test_name} COMMAND ${target_name})
  endfunction()

  add_helmet_test(default)
  add_helmet_test(custom)
  add_helmet_test(disable)
endif()
