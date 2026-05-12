cmake_minimum_required(VERSION 3.13.1)

# Configurazione WAMR — solo variabili, NON include ancora
set(WAMR_BUILD_PLATFORM "zephyr")
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_LIBC_WASI 0)
set(WAMR_BUILD_FAST_INTERP 0)
set(WAMR_BUILD_GLOBAL_HEAP_POOL 1)
set(WAMR_BUILD_GLOBAL_HEAP_SIZE 131072)
set(WAMR_ROOT_DIR $ENV{WAMR_ROOT} CACHE PATH "Path to WAMR root directory")

list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/modules/libmicroros)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(microros_wamr_app)

# Include WAMR DOPO find_package(Zephyr) — toolchain ARM già attivo
include(${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)

target_sources(app PRIVATE
    ${WAMR_RUNTIME_LIB_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/wasm_export_native.c
)

target_link_libraries(app PUBLIC microros)

target_include_directories(app PRIVATE
    ${APP_INC_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)