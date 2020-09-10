//
// Created by noisyfox on 2020/9/10.
//

#include "jni.h"

jfieldID g_field_java_io_FileOutputStream_fd;

JNIEXPORT void JNICALL Java_java_io_FileOutputStream_initIDs(JNIEnv *env, jclass cls) {
    g_field_java_io_FileOutputStream_fd = (*env)->GetFieldID(env, cls, "fd", "Ljava/io/FileDescriptor;");
}
