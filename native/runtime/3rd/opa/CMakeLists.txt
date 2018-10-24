project(OpenPA C)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(OPA_HAVE_PTHREAD_H ${CMAKE_USE_PTHREADS_INIT})

include(configure)

configure_file(opa_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/opa_config.h)

set(LIBRARIES_OPA)

# Set thread implementation
if (OPA_HAVE_PTHREAD_H)
    list(APPEND LIBRARIES_OPA Threads::Threads)
endif ()

add_c_library(
        opa
        OUTPUT_NAME opa
        PKGCONFIG_NAME opa
        VERSION 0.0.1

        SOURCES
        opa_primitives.c
        opa_queue.c

        LIBRARIES
        ${LIBRARIES_OPA}
)
