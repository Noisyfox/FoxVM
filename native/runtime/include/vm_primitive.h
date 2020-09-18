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

JAVA_VOID primitive_init(VM_PARAM_CURRENT_CONTEXT);

JAVA_CLASS primitive_of_name(C_CSTR name);

JAVA_BYTE primitive_unbox_b(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT o);
JAVA_CHAR primitive_unbox_c(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT o);
JAVA_DOUBLE primitive_unbox_d(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT o);
JAVA_FLOAT primitive_unbox_f(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT o);
JAVA_INT primitive_unbox_i(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT o);
JAVA_LONG primitive_unbox_j(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT o);
JAVA_SHORT primitive_unbox_s(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT o);
JAVA_BOOLEAN primitive_unbox_z(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT o);

#endif //FOXVM_VM_PRIMITIVE_H
