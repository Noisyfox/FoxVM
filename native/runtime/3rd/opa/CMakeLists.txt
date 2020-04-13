project(OpenPA C)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

set(OPA_HAVE_PTHREAD_H ${CMAKE_USE_PTHREADS_INIT})

include(configure.cmake)

configure_file(opa_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/opa_config.h)

set(LOCAL_LIBRARIES_OPA)

# Set thread implementation
if (OPA_HAVE_PTHREAD_H)
    list(APPEND LOCAL_LIBRARIES_OPA Threads::Threads)
endif ()

add_c_library(
        opa
        OUTPUT_NAME opa
        PKGCONFIG_NAME opa
        VERSION 0.0.1

        SOURCES
        opa_primitives.c
        opa_queue.c

        LOCAL_LIBRARIES
        ${LOCAL_LIBRARIES_OPA}
)
