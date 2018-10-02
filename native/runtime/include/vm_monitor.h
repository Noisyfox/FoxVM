//
// Created by noisyfox on 2018/10/1.
//

#ifndef FOXVM_VM_MONITOR_H
#define FOXVM_VM_MONITOR_H

#include "vm_base.h"

JAVA_VOID monitorCreate(VMThreadContext *ctx, JAVA_OBJECT obj);

JAVA_VOID monitorFree(VMThreadContext *ctx, JAVA_OBJECT obj);

JAVA_VOID monitorEnter(VMThreadContext *ctx, JAVA_OBJECT obj);

JAVA_VOID monitorExit(VMThreadContext *ctx, JAVA_OBJECT obj);

JAVA_VOID monitorWait(VMThreadContext *ctx, JAVA_OBJECT obj, JAVA_LONG timeout, JAVA_INT nanos);

JAVA_VOID monitorNotify(VMThreadContext *ctx, JAVA_OBJECT obj);

JAVA_VOID monitorNotifyAll(VMThreadContext *ctx, JAVA_OBJECT obj);

#endif //FOXVM_VM_MONITOR_H
