# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.14)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wamr_led_toggle)
enable_language(ASM)

set(WAMR_BUILD_PLATFORM "zephyr")
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_LIBC_WASI 0)
set(WAMR_BUILD_FAST_INTERP 0)
set(WAMR_BUILD_GLOBAL_HEAP_POOL 1)
set(WAMR_BUILD_GLOBAL_HEAP_SIZE 131072)

set(WAMR_ROOT_DIR $ENV{WAMR_ROOT} CACHE PATH "Path to WAMR root directory")

include(${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)

target_sources(app PRIVATE
    ${WAMR_RUNTIME_LIB_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/wasm_export_native.c
)

target_include_directories(app PRIVATE
    ${APP_INC_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)