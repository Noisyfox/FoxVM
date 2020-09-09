//
// Created by noisyfox on 2018/9/23.
//

#ifndef FOXVM_VM_EXCEPTION_H
#define FOXVM_VM_EXCEPTION_H

#include <setjmp.h>
#include "vm_stack.h"

typedef struct _ExceptionFrame {
    struct _ExceptionFrame *previous;

    jmp_buf jmpTarget;
} ExceptionFrame;

#define exception_frame_start(block_number)

#define exception_new_block(index, start, end, handler, type)

#endif //FOXVM_VM_EXCEPTION_H
