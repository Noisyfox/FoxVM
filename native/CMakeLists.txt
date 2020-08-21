cmake_minimum_required(VERSION 3.1.0)
project(foxvm C)
set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=implicit-function-declaration -Werror=implicit-int -Werror=incompatible-pointer-types -Werror=return-type -Werror=shadow -Werror=return-local-addr -Werror=uninitialized -Werror=format -Werror=sign-compare -Werror=int-conversion")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Werror=implicit-function-declaration -Werror=implicit-int -Werror=incompatible-pointer-types -Werror=return-type -Werror=shadow -Werror=return-local-addr -Werror=uninitialized -Werror=format -Werror=sign-compare -Werror=int-conversion")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(CTargets)

add_subdirectory(runtime)
include_directories($<TARGET_PROPERTY:foxvm_runtime,INCLUDE_DIRECTORIES>)

add_subdirectory(testGen)

add_executable(foxvm main.c)

target_link_libraries(foxvm
        foxvm_runtime
        foxvm_app
        )