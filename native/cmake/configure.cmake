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
