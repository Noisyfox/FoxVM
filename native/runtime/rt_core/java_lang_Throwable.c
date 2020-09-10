//
// Created by noisyfox on 2020/9/10.
//

#include "jni.h"

JNIEXPORT jobject JNICALL Java_java_lang_Throwable_fillInStackTrace(JNIEnv *env, jobject throwable, jint dummy) {
    return throwable;
}

JNIEXPORT jint JNICALL Java_java_lang_Throwable_getStackTraceDepth(JNIEnv *env, jobject throwable) {
    return 0;
}
