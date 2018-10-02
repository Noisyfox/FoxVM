project(foxvm_runtime C)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(HAVE_PTHREADS ${CMAKE_USE_PTHREADS_INIT})

include ( configure )

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/config.h )

add_c_library(
        foxvm_runtime
        OUTPUT_NAME runtime
        PKGCONFIG_NAME foxvm_runtime
        VERSION 0.0.1

        SOURCES
        vm_base.c
        vm_monitor.c
        vm_exception.c
        vm_memory.c

        vm_main.c

        LIBRARIES
        Threads::Threads
)
