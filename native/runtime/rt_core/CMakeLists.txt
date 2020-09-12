set(SOURCE_FILES_RT_CORE
        java_lang_Class.c
        java_lang_Object.c
        java_lang_Float.c
        java_lang_Double.c
        java_lang_System.c
        java_lang_Thread.c
        java_lang_Throwable.c
        java_lang_reflect_Array.c
        sun_misc_Unsafe.c
        sun_reflect_Reflection.c
        )


if (HAVE_UNISTD_H)
    list(APPEND SOURCE_FILES_RT_CORE
            posix/java_io_FileDescriptor.c
            posix/java_io_FileOutputStream.c
            )
elseif (WIN32 AND NOT CYGWIN)
else ()
    message(FATAL_ERROR "Not supported on this platform.")
endif ()

include_directories($<TARGET_PROPERTY:foxvm_runtime,INCLUDE_DIRECTORIES>)

add_library(foxvm_rt_core
        OBJECT
        ${SOURCE_FILES_RT_CORE}
)
