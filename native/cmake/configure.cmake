# Build args

# Platform checks
include ( CheckFunctionExists )
include ( CheckIncludeFiles )
include ( CheckSymbolExists )
include ( CheckCSourceCompiles )
include ( CheckTypeSize )
include ( CheckSTDC )

check_include_files ( "sys/mman.h" HAVE_MMAN_H )

check_symbol_exists ( _aligned_malloc "malloc.h" HAVE_ALIGNED_MALLOC )
check_symbol_exists ( posix_memalign "stdlib.h" HAVE_POSIX_MEMALIGN )
check_symbol_exists ( get_nprocs "sys/sysinfo.h" HAVE_GET_NPROCS )
check_symbol_exists ( _SC_NPROCESSORS_ONLN "unistd.h" HAVE_SC_NPROCESSORS_ONLN )
check_symbol_exists ( HW_AVAILCPU "unistd.h" HAVE_HW_AVAILCPU )
