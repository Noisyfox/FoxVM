project(foxvm_runtime C)

add_c_library(
        foxvm_runtime
        OUTPUT_NAME runtime
        PKGCONFIG_NAME foxvm_runtime
        VERSION 0.0.1
        SOURCES
        vm_main.c
        vm_exception.c
        vm_memory.c
)
