cmake_minimum_required(VERSION 3.11)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source build is not allowed")
endif()

project(wasiapp)

set(OUT_FILE "app.out")

if(BUILD_DUMMY)
  set(app_srcs src/dummy.c)
else()
  set(app_srcs src/wasi-app.c src/wasi-main.c src/wasm-rt-impl.c)
endif()
add_executable(${OUT_FILE} ${app_srcs})

# Set options

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "set build type to Release" FORCE)
endif()

include(FetchContent)
include(CheckIPOSupported)

FetchContent_Declare(libuv  SOURCE_DIR ${CMAKE_SOURCE_DIR}/deps/libuv)
FetchContent_Declare(uvwasi SOURCE_DIR ${CMAKE_SOURCE_DIR}/deps/uvwasi)

FetchContent_GetProperties(uvwasi)
if(NOT uvwasi_POPULATED)
  FetchContent_Populate(uvwasi)
  include_directories("${uvwasi_SOURCE_DIR}/include")
  add_subdirectory(${uvwasi_SOURCE_DIR} ${uvwasi_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-O0")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-O3")

target_link_libraries(${OUT_FILE} uvwasi_a uv_a m)

check_ipo_supported(RESULT result)
if(result)
  set_property(TARGET ${OUT_FILE} PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
endif()
