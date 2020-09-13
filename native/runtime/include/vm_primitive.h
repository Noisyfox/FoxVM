//
// Created by noisyfox on 2020/8/30.
//

#ifndef FOXVM_VM_PRIMITIVE_H
#define FOXVM_VM_PRIMITIVE_H

#include "vm_base.h"

extern JavaClassInfo g_classInfo_primitive_Z;
extern JavaClassInfo g_classInfo_primitive_B;
extern JavaClassInfo g_classInfo_primitive_C;
extern JavaClassInfo g_classInfo_primitive_S;
extern JavaClassInfo g_classInfo_primitive_I;
extern JavaClassInfo g_classInfo_primitive_J;
extern JavaClassInfo g_classInfo_primitive_F;
extern JavaClassInfo g_classInfo_primitive_D;
extern JavaClassInfo g_classInfo_primitive_V;

extern JAVA_CLASS g_class_primitive_Z;
extern JAVA_CLASS g_class_primitive_B;
extern JAVA_CLASS g_class_primitive_C;
extern JAVA_CLASS g_class_primitive_S;
extern JAVA_CLASS g_class_primitive_I;
extern JAVA_CLASS g_class_primitive_J;
extern JAVA_CLASS g_class_primitive_F;
extern JAVA_CLASS g_class_primitive_D;
extern JAVA_CLASS g_class_primitive_V;

JAVA_CLASS primitive_of_name(C_CSTR name);

#endif //FOXVM_VM_PRIMITIVE_H
