//
// Created by noisyfox on 2018/9/23.
//

#ifndef FOXVM_VM_EXCEPTION_H
#define FOXVM_VM_EXCEPTION_H

#include <setjmp.h>

typedef struct _ExceptionFrame {
    struct _ExceptionFrame *previous;

    jmp_buf jmpTarget;
} ExceptionFrame;

#endif //FOXVM_VM_EXCEPTION_H
