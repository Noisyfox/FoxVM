project(foxvm_runtime C)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(HAVE_PTHREADS ${CMAKE_USE_PTHREADS_INIT})

include(configure.cmake)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)

set(SOURCE_FILES_RUNTIME
        vm_base.c
        vm_array.c
        vm_class.c
        vm_primitive.c
        vm_exception.c
        vm_stack.c
        vm_field.c
        vm_bytecode.c
        vm_string.c
        vm_method.c
        vm_reflection.c

        thread/vm_thread.c

        memory/vm_memory.c
        memory/vm_tlab.c
        memory/vm_gc.c
        memory/vm_gc_priv.c

        classloader/vm_classloader.c
        classloader/vm_boot_classloader.c

        native/vm_native.c
        )

set(LOCAL_LIBS
        Threads::Threads
        m
        opa
        )

# Set thread implementation
if (HAVE_PTHREADS)
    list(APPEND SOURCE_FILES_RUNTIME thread/vm_thread_posix.c)
elseif (WIN32 AND NOT CYGWIN)
    list(APPEND SOURCE_FILES_RUNTIME thread/vm_thread_win32.c)
else ()
    message(FATAL_ERROR "Not supported on this platform.")
endif ()

if (HAVE_MMAN_H)
    list(APPEND SOURCE_FILES_RUNTIME memory/vm_memory_posix.c)
elseif (WIN32 AND NOT CYGWIN)
    list(APPEND SOURCE_FILES_RUNTIME memory/vm_memory_win32.c)
else ()
    message(FATAL_ERROR "Not supported on this platform.")
endif ()

if (HAVE_DLFCN_H)
    list(APPEND SOURCE_FILES_RUNTIME native/vm_native_posix.c)
    list(APPEND LOCAL_LIBS ${CMAKE_DL_LIBS})
else ()
    message(FATAL_ERROR "Not supported on this platform.")
endif ()

add_subdirectory(3rd)

include_directories(
        $<TARGET_PROPERTY:opa,INCLUDE_DIRECTORIES>
        3rd/uthash
)

add_subdirectory(rt_core)

add_c_library(
        foxvm_runtime
        OUTPUT_NAME runtime
        PKGCONFIG_NAME foxvm_runtime
        VERSION 0.0.1

        SOURCES
        ${SOURCE_FILES_RUNTIME}

        vm_main.c

        LIBRARIES

        LOCAL_LIBRARIES
        ${LOCAL_LIBS}
)
