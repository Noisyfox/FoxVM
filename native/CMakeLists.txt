cmake_minimum_required(VERSION 2.6)
project(foxvm C)
set(CMAKE_C_STANDARD 99)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(CTargets)

add_subdirectory(runtime)
include_directories($<TARGET_PROPERTY:foxvm_runtime,INCLUDE_DIRECTORIES>)

add_executable(foxvm main.c)

target_link_libraries(foxvm
        foxvm_runtime
        )