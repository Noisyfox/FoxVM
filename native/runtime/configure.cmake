# Build args

# Platform checks
include ( CheckFunctionExists )
include ( CheckIncludeFiles )
include ( CheckSymbolExists )
include ( CheckCSourceCompiles )
include ( CheckTypeSize )
include ( CheckSTDC )

check_include_files ( "sys/mman.h" HAVE_MMAN_H )
check_include_files ( "dlfcn.h" HAVE_DLFCN_H )

check_symbol_exists ( _aligned_malloc "malloc.h" HAVE_ALIGNED_MALLOC )
check_symbol_exists ( posix_memalign "stdlib.h" HAVE_POSIX_MEMALIGN )
check_symbol_exists ( get_nprocs "sys/sysinfo.h" HAVE_GET_NPROCS )
check_symbol_exists ( _SC_NPROCESSORS_ONLN "unistd.h" HAVE_SC_NPROCESSORS_ONLN )
check_symbol_exists ( HW_AVAILCPU "sys/sysctl.h" HAVE_HW_AVAILCPU )

# Platform detect code copied from
# https://github.com/dotnet/runtime/blob/21a7f5e1a6538214804fb6ed2dbbed28d025ed3b/eng/native/configureplatform.cmake
# and
# https://github.com/dotnet/runtime/blob/21a7f5e1a6538214804fb6ed2dbbed28d025ed3b/eng/native/configurecompiler.cmake

function(vm_unknown_arch)
    if (WIN32)
        message(FATAL_ERROR "Only AMD64, ARM64, ARM and I386 are supported. Found: ${CMAKE_SYSTEM_PROCESSOR}")
    else()
        message(FATAL_ERROR "Only AMD64, ARM64 and ARM are supported. Found: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endfunction()

set(VM_CMAKE_HOST_OS ${CMAKE_SYSTEM_NAME})
if(VM_CMAKE_HOST_OS STREQUAL Linux)
    set(VM_CMAKE_HOST_UNIX 1)
    set(VM_CMAKE_HOST_LINUX 1)
    # CMAKE_SYSTEM_PROCESSOR returns the value of `uname -p` on target.
    # For the AMD/Intel 64bit architecture two different strings are common.
    # Linux and Darwin identify it as "x86_64" while FreeBSD and netbsd uses the
    # "amd64" string. Accept either of the two here.
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64 OR CMAKE_SYSTEM_PROCESSOR STREQUAL amd64)
        set(VM_CMAKE_HOST_UNIX_AMD64 1)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL armv7l)
        set(VM_CMAKE_HOST_UNIX_ARM 1)
        set(VM_CMAKE_HOST_UNIX_ARMV7L 1)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL arm OR CMAKE_SYSTEM_PROCESSOR STREQUAL armv7-a)
        set(VM_CMAKE_HOST_UNIX_ARM 1)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL aarch64)
        set(VM_CMAKE_HOST_UNIX_ARM64 1)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL i686)
        set(VM_CMAKE_HOST_UNIX_X86 1)
    else()
        vm_unknown_arch()
    endif()
endif(VM_CMAKE_HOST_OS STREQUAL Linux)

if(VM_CMAKE_HOST_OS STREQUAL Darwin)
    set(VM_CMAKE_HOST_UNIX 1)
    set(VM_CMAKE_HOST_UNIX_AMD64 1)
    set(VM_CMAKE_HOST_OSX 1)
#    set(CMAKE_ASM_COMPILE_OBJECT "${CMAKE_C_COMPILER} <FLAGS> <DEFINES> <INCLUDES> -o <OBJECT> -c <SOURCE>")
endif(VM_CMAKE_HOST_OS STREQUAL Darwin)

if(VM_CMAKE_HOST_OS STREQUAL iOS)
    set(VM_CMAKE_HOST_UNIX 1)
    set(VM_CMAKE_HOST_IOS 1)
    if(CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
        set(VM_CMAKE_HOST_UNIX_AMD64 1)
    elseif(CMAKE_OSX_ARCHITECTURES MATCHES "armv7")
        set(VM_CMAKE_HOST_UNIX_ARM 1)
    elseif(CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
        set(VM_CMAKE_HOST_UNIX_ARM64 1)
    else()
        vm_unknown_arch()
    endif()
endif(VM_CMAKE_HOST_OS STREQUAL iOS)

if(VM_CMAKE_HOST_OS STREQUAL tvOS)
    set(VM_CMAKE_HOST_UNIX 1)
    set(VM_CMAKE_HOST_TVOS 1)
    if(CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
        set(VM_CMAKE_HOST_UNIX_AMD64 1)
    elseif(CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
        set(VM_CMAKE_HOST_UNIX_ARM64 1)
    else()
        vm_unknown_arch()
    endif()
endif(VM_CMAKE_HOST_OS STREQUAL tvOS)

# Set HOST architecture variables
if(VM_CMAKE_HOST_UNIX_ARM)
    set(VM_CMAKE_HOST_ARCH_ARM 1)
    set(VM_CMAKE_HOST_ARCH "arm")

    if(VM_CMAKE_HOST_UNIX_ARMV7L)
        set(VM_CMAKE_HOST_ARCH_ARMV7L 1)
    endif()
elseif(VM_CMAKE_HOST_UNIX_ARM64)
    set(VM_CMAKE_HOST_ARCH_ARM64 1)
    set(VM_CMAKE_HOST_ARCH "arm64")
elseif(VM_CMAKE_HOST_UNIX_AMD64)
    set(VM_CMAKE_HOST_ARCH_AMD64 1)
    set(VM_CMAKE_HOST_ARCH "x64")
elseif(VM_CMAKE_HOST_UNIX_X86)
    set(VM_CMAKE_HOST_ARCH_I386 1)
    set(VM_CMAKE_HOST_ARCH "x86")
elseif(WIN32)
    # VM_CMAKE_HOST_ARCH is passed in as param to cmake
    if (VM_CMAKE_HOST_ARCH STREQUAL x64)
        set(VM_CMAKE_HOST_ARCH_AMD64 1)
    elseif(VM_CMAKE_HOST_ARCH STREQUAL x86)
        set(VM_CMAKE_HOST_ARCH_I386 1)
    elseif(VM_CMAKE_HOST_ARCH STREQUAL arm)
        set(VM_CMAKE_HOST_ARCH_ARM 1)
    elseif(VM_CMAKE_HOST_ARCH STREQUAL arm64)
        set(VM_CMAKE_HOST_ARCH_ARM64 1)
    else()
        vm_unknown_arch()
    endif()
endif()

# Set TARGET architecture variables
# Target arch will be a cmake param (optional) for both windows as well as non-windows build
# if target arch is not specified then host & target are same
if(NOT DEFINED VM_CMAKE_TARGET_ARCH OR VM_CMAKE_TARGET_ARCH STREQUAL "" )
    set(VM_CMAKE_TARGET_ARCH ${VM_CMAKE_HOST_ARCH})

    # This is required for "arm" targets (CMAKE_SYSTEM_PROCESSOR "armv7l"),
    # for which this flag otherwise won't be set up below
    if (VM_CMAKE_HOST_ARCH_ARMV7L)
        set(VM_CMAKE_TARGET_ARCH_ARMV7L 1)
    endif()
endif()

# Set target architecture variables
if (VM_CMAKE_TARGET_ARCH STREQUAL x64)
    set(VM_CMAKE_TARGET_ARCH_AMD64 1)
elseif(VM_CMAKE_TARGET_ARCH STREQUAL x86)
    set(VM_CMAKE_TARGET_ARCH_I386 1)
elseif(VM_CMAKE_TARGET_ARCH STREQUAL arm64)
    set(VM_CMAKE_TARGET_ARCH_ARM64 1)
elseif(VM_CMAKE_TARGET_ARCH STREQUAL arm)
    set(VM_CMAKE_TARGET_ARCH_ARM 1)
endif()

# Set TARGET architecture variables
# Target os will be a cmake param (optional) for both windows as well as non-windows build
# if target os is not specified then host & target os are same
if (NOT DEFINED VM_CMAKE_TARGET_OS OR VM_CMAKE_TARGET_OS STREQUAL "" )
    set(VM_CMAKE_TARGET_OS ${VM_CMAKE_HOST_OS})
endif()

if(VM_CMAKE_TARGET_OS STREQUAL Linux)
    set(VM_CMAKE_TARGET_UNIX 1)
    set(VM_CMAKE_TARGET_LINUX 1)
endif(VM_CMAKE_TARGET_OS STREQUAL Linux)

if(VM_CMAKE_TARGET_OS STREQUAL Android)
    set(VM_CMAKE_TARGET_UNIX 1)
    set(VM_CMAKE_TARGET_LINUX 1)
    set(VM_CMAKE_TARGET_ANDROID 1)
endif(VM_CMAKE_TARGET_OS STREQUAL Android)

if(VM_CMAKE_TARGET_OS STREQUAL Darwin)
    set(VM_CMAKE_TARGET_UNIX 1)
    set(VM_CMAKE_TARGET_OSX 1)
endif(VM_CMAKE_TARGET_OS STREQUAL Darwin)

if(VM_CMAKE_TARGET_OS STREQUAL iOS)
    set(VM_CMAKE_TARGET_UNIX 1)
    set(VM_CMAKE_TARGET_IOS 1)
endif(VM_CMAKE_TARGET_OS STREQUAL iOS)

if(VM_CMAKE_TARGET_OS STREQUAL tvOS)
    set(VM_CMAKE_TARGET_UNIX 1)
    set(VM_CMAKE_TARGET_TVOS 1)
endif(VM_CMAKE_TARGET_OS STREQUAL tvOS)

if(VM_CMAKE_TARGET_UNIX)
    if(VM_CMAKE_TARGET_ARCH STREQUAL x64)
        set(VM_CMAKE_TARGET_UNIX_AMD64 1)
    elseif(VM_CMAKE_TARGET_ARCH STREQUAL arm)
        set(VM_CMAKE_TARGET_UNIX_ARM 1)
    elseif(VM_CMAKE_TARGET_ARCH STREQUAL arm64)
        set(VM_CMAKE_TARGET_UNIX_ARM64 1)
    elseif(VM_CMAKE_TARGET_ARCH STREQUAL x86)
        set(VM_CMAKE_TARGET_UNIX_X86 1)
    else()
        vm_unknown_arch()
    endif()
else()
    set(VM_CMAKE_TARGET_WIN32 1)
endif(VM_CMAKE_TARGET_UNIX)

# check if host & target os/arch combination are valid
if (VM_CMAKE_TARGET_OS STREQUAL VM_CMAKE_HOST_OS)
    if(NOT(VM_CMAKE_TARGET_ARCH STREQUAL VM_CMAKE_HOST_ARCH))
        if(NOT((VM_CMAKE_HOST_ARCH_AMD64 AND VM_CMAKE_TARGET_ARCH_ARM64) OR (VM_CMAKE_HOST_ARCH_I386 AND VM_CMAKE_TARGET_ARCH_ARM) OR (VM_CMAKE_HOST_ARCH_AMD64 AND VM_CMAKE_TARGET_ARCH_ARM)))
            message(FATAL_ERROR "Invalid platform and target arch combination")
        endif()
    endif()
else()
    if(NOT (VM_CMAKE_HOST_OS STREQUAL Windows_NT))
        message(FATAL_ERROR "Invalid host and target os/arch combination. Host OS: ${VM_CMAKE_HOST_OS}")
    endif()
    if(NOT ((VM_CMAKE_HOST_ARCH_AMD64 AND (VM_CMAKE_TARGET_ARCH_AMD64 OR VM_CMAKE_TARGET_ARCH_ARM64)) OR (VM_CMAKE_HOST_ARCH_I386 AND VM_CMAKE_TARGET_ARCH_ARM)))
        message(FATAL_ERROR "Invalid host and target os/arch combination. Host Arch: ${VM_CMAKE_HOST_ARCH} Target Arch: ${VM_CMAKE_TARGET_ARCH}")
    endif()
endif()

#-----------------------------------
# Definitions (for platform)
#-----------------------------------
# Host
if (VM_CMAKE_HOST_ARCH_AMD64)
    set(HOST_AMD64 1)
    set(HOST_64BIT 1)
elseif (VM_CMAKE_HOST_ARCH_I386)
    set(HOST_X86 1)
elseif (VM_CMAKE_HOST_ARCH_ARM)
    set(HOST_ARM 1)
elseif (VM_CMAKE_HOST_ARCH_ARM64)
    set(HOST_ARM64 1)
    set(HOST_64BIT 1)
else ()
    vm_unknown_arch()
endif ()

if (VM_CMAKE_HOST_UNIX)
    set(HOST_UNIX 1)

    if(VM_CMAKE_HOST_LINUX)
        if(VM_CMAKE_HOST_UNIX_AMD64)
            message("Detected Linux x86_64")
        elseif(VM_CMAKE_HOST_UNIX_ARM)
            message("Detected Linux ARM")
        elseif(VM_CMAKE_HOST_UNIX_ARM64)
            message("Detected Linux ARM64")
        elseif(VM_CMAKE_HOST_UNIX_X86)
            message("Detected Linux i686")
        else()
            vm_unknown_arch()
        endif()
    endif(VM_CMAKE_HOST_LINUX)
    if(VM_CMAKE_HOST_OSX)
        message("Detected OSX x86_64")
    endif(VM_CMAKE_HOST_OSX)
endif(VM_CMAKE_HOST_UNIX)

if (VM_CMAKE_HOST_WIN32)
    set(HOST_WINDOWS 1)
endif(VM_CMAKE_HOST_WIN32)

# Target
if (VM_CMAKE_TARGET_ARCH_AMD64)
    set(TARGET_AMD64 1)
    set(TARGET_64BIT 1)
elseif (VM_CMAKE_TARGET_ARCH_ARM64)
    set(TARGET_ARM64 1)
    set(TARGET_64BIT 1)
elseif (VM_CMAKE_TARGET_ARCH_ARM)
    set(TARGET_ARM 1)
elseif (VM_CMAKE_TARGET_ARCH_I386)
    set(TARGET_X86 1)
else ()
    vm_unknown_arch()
endif ()

if (VM_CMAKE_TARGET_UNIX)
    set(TARGET_UNIX 1)
endif (VM_CMAKE_TARGET_UNIX)

if (VM_CMAKE_TARGET_WIN32)
    set(TARGET_WINDOWS 1)
endif(VM_CMAKE_TARGET_WIN32)
