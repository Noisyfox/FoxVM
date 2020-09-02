set(SOURCE_FILES_RT_CORE
        java_lang_Class.c)

include_directories($<TARGET_PROPERTY:foxvm_runtime,INCLUDE_DIRECTORIES>)

add_library(foxvm_rt_core
        OBJECT
        ${SOURCE_FILES_RT_CORE}
)
