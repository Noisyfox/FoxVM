//
// Created by noisyfox on 2020/9/2.
//

#include "jni.h"

union Float {
    jint bits;
    jfloat f;
};

JNIEXPORT jint Java_java_lang_Float_floatToRawIntBits(JNIEnv *env, jclass cls, jfloat val) {
    union Float f;
    f.f = val;

    return f.bits;
}

JNIEXPORT jfloat Java_java_lang_Float_intBitsToFloat(JNIEnv *env, jclass cls, jint val) {
    union Float f;
    f.bits = val;

    return f.f;
}
