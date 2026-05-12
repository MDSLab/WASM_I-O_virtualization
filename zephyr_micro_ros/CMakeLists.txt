cmake_minimum_required(VERSION 3.20.0)

list(APPEND ZEPHYR_EXTRA_MODULES
    /home/giovanni/zephyrproject/modules/lib/micro_ros
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(microros_publisher)

target_sources(app PRIVATE src/main.c)