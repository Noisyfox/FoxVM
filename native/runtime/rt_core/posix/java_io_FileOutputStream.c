//
// Created by noisyfox on 2020/9/10.
//

#include "jni.h"
#include "java_io_FileDescriptor.h"
#include "vm_native.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define BUF_SIZE 8192

static jfieldID g_field_java_io_FileOutputStream_fd;

JNIEXPORT void JNICALL Java_java_io_FileOutputStream_initIDs(JNIEnv *env, jclass cls) {
    g_field_java_io_FileOutputStream_fd = (*env)->GetFieldID(env, cls, "fd", "Ljava/io/FileDescriptor;");
}

static jint java_io_FileOutputStream_getFD(JNIEnv *env, jobject obj) {
    jobject fdObj = (*env)->GetObjectField(env, obj, g_field_java_io_FileOutputStream_fd);

    if (!fdObj) {
        return -1;
    }

    return java_io_FileDescriptor_getFD(env, fdObj);
}

/**
 * Retry the operation if it is interrupted
 */
#define RESTARTABLE(_cmd, _result) do {             \
    do {                                            \
        _result = _cmd;                             \
    } while((_result == -1) && (errno == EINTR));   \
} while(0)

static ssize_t handleWrite(jint fd, const void *buf, jint len) {
    ssize_t result;
    RESTARTABLE(write(fd, buf, len), result);
    return result;
}

JNIEXPORT void JNICALL Java_java_io_FileOutputStream_writeBytes(JNIEnv *env, jobject obj, jbyteArray b, jint off, jint len, jboolean append) {
    char stackBuf[BUF_SIZE];
    char *buf = NULL;

    if (!b) {
        // TODO: throw NullPointerException instead.
        abort();
    }

    if (off < 0 || len < 0 || (*env)->GetArrayLength(env, b) - off < len) {
        // TODO: throw IndexOutOfBoundsException instead.
        abort();
    }

    if (len == 0) {
        return;
    } else if (len > BUF_SIZE) {
        buf = malloc(len);
        if (buf == NULL) {
            // TODO: throw OOM
            abort();
        }
    } else {
        buf = stackBuf;
    }

    (*env)->GetByteArrayRegion(env, b, off, len, (jbyte *) buf);
    // TODO: check exception

    off = 0;
    while (len > 0) {
        jint fd = java_io_FileOutputStream_getFD(env, obj);
        if (fd == -1) {
            // TODO: throw IOException instead.
            fprintf(stderr, "FileOutputStream: closed\n");
            abort();
        }

        jint n = handleWrite(fd, buf + off, len);
        if (n == -1) {
            // TODO: throw IOException instead.
            fprintf(stderr, "FileOutputStream: write error: %d\n", n);
            abort();
        }
        off += n;
        len -= n;
    }
    if (buf != stackBuf) {
        free(buf);
    }
}
