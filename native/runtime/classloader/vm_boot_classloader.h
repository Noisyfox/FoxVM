//
// Created by noisyfox on 2020/8/27.
//

#ifndef FOXVM_VM_BOOT_CLASSLOADER_H
#define FOXVM_VM_BOOT_CLASSLOADER_H

#include "vm_base.h"

#define cached_class(var)                \
extern JavaClassInfo *g_classInfo_##var; \
extern JAVA_CLASS     g_class_##var

cached_class(java_lang_Object);
cached_class(java_lang_Class);
cached_class(java_lang_ClassLoader);
cached_class(java_lang_String);
cached_class(java_lang_Boolean);
cached_class(java_lang_Byte);
cached_class(java_lang_Character);
cached_class(java_lang_Short);
cached_class(java_lang_Integer);
cached_class(java_lang_Long);
cached_class(java_lang_Float);
cached_class(java_lang_Double);
cached_class(java_lang_Enum);
cached_class(java_lang_Cloneable);
cached_class(java_lang_Thread);
cached_class(java_lang_ThreadGroup);
cached_class(java_io_Serializable);
cached_class(java_lang_Runtime);

cached_class(java_lang_ClassNotFoundException);
cached_class(java_lang_NoClassDefFoundError);
cached_class(java_lang_Error);

#undef cached_class

JAVA_BOOLEAN cl_bootstrap_init(VM_PARAM_CURRENT_CONTEXT);

JAVA_CLASS cl_bootstrap_find_class_by_info(VM_PARAM_CURRENT_CONTEXT, JavaClassInfo *classInfo);

JAVA_CLASS cl_bootstrap_find_class(VM_PARAM_CURRENT_CONTEXT, C_CSTR className);

#endif //FOXVM_VM_BOOT_CLASSLOADER_H
