cmake_minimum_required(VERSION 3.14)
project(ecewo_helmet VERSION 0.1.0 LANGUAGES C)

add_library(ecewo_helmet
  src/ecewo-helmet.c
)

add_library(ecewo::helmet ALIAS ecewo_helmet)

target_include_directories(ecewo_helmet PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

if (PROJECT_IS_TOP_LEVEL AND NOT TARGET ecewo::ecewo)
  include(FetchContent)

  FetchContent_Declare(
    ecewo
    GIT_REPOSITORY https://github.com/savashn/ecewo.git
    GIT_TAG        dev
    GIT_SHALLOW    TRUE
  )

  FetchContent_MakeAvailable(ecewo)
endif()

if (PROJECT_IS_TOP_LEVEL AND TARGET ecewo::ecewo)
  ecewo_plugin(mock)
  enable_testing()

  add_executable(ecewo_helmet_test
    tests/test.c
    tests/runner.c
  )

  target_include_directories(ecewo_helmet_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
  )

  target_link_libraries(ecewo_helmet_test
    PRIVATE
      ecewo::ecewo
      ecewo::helmet
      ecewo::mock
  )

  if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ecewo_helmet_test PRIVATE
      -Wall
      -Wextra
      -Werror
      -Wstrict-prototypes
      -Wincompatible-pointer-types
      -Wno-unused-parameter
    )
  endif()

  add_test(
    NAME ECEWO_HELMET_all
    COMMAND ecewo_helmet_test
  )
endif()

target_link_libraries(ecewo_helmet PUBLIC ecewo::ecewo)
