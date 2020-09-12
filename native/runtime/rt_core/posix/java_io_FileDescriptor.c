//
// Created by noisyfox on 2020/9/10.
//

#include "java_io_FileDescriptor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static jfieldID g_field_java_io_FileDescriptor_fd;

JNIEXPORT void JNICALL Java_java_io_FileDescriptor_initIDs(JNIEnv *env, jclass cls) {
    g_field_java_io_FileDescriptor_fd = (*env)->GetFieldID(env, cls, "fd", "I");
}

jint java_io_FileDescriptor_getFD(JNIEnv *env, jobject obj) {
    return (*env)->GetIntField(env, obj, g_field_java_io_FileDescriptor_fd);
}

JNIEXPORT void JNICALL Java_java_io_FileDescriptor_sync(JNIEnv *env, jobject obj) {
    jint fd = java_io_FileDescriptor_getFD(env, obj);

    if (fsync(fd) == -1) {
        // TODO: throw SyncFailedException instead.
        fprintf(stderr, "FileDescriptor: sync failed\n");
        abort();
    }
}
