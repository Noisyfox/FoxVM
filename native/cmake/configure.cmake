# Build args

# Platform checks
include ( CheckFunctionExists )
include ( CheckIncludeFiles )
include ( CheckSymbolExists )
include ( CheckCSourceCompiles )
include ( CheckTypeSize )
include ( CheckSTDC )


check_include_files ( semaphore.h HAVE_SEMAPHORE_H )

ADD_DEFINITIONS(-DHAVE_CONFIG_H)
