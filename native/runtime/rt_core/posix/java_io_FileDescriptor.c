//
// Created by noisyfox on 2020/9/10.
//

#include "jni.h"

jfieldID g_field_java_io_FileDescriptor_fd;

JNIEXPORT void JNICALL Java_java_io_FileDescriptor_initIDs(JNIEnv *env, jclass cls) {
    g_field_java_io_FileDescriptor_fd = (*env)->GetFieldID(env, cls, "fd", "I");
}
