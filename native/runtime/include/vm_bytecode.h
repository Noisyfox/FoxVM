//
// Created by noisyfox on 2020/5/10.
//

#ifndef FOXVM_VM_BYTECODE_H
#define FOXVM_VM_BYTECODE_H

#include "vm_stack.h"


/**
 * Instruction `aconst_null`.
 * Push the `null` object reference onto the operand stack.
 */
#define bc_aconst_null() stack_push_object(JAVA_NULL)


// FoxVM specific instructions

/** Record current source file line number. */
#define bc_line(line_number) __stackFrame.currentLine = (line_number)

#endif //FOXVM_VM_BYTECODE_H
