//
// Created by noisyfox on 2020/9/2.
//

#include "jni.h"

union Double {
    jlong bits;
    jdouble d;
};

JNIEXPORT jlong Java_java_lang_Double_doubleToRawLongBits(JNIEnv *env, jclass cls, jdouble val) {
    union Double d;
    d.d = val;

    return d.bits;
}

JNIEXPORT jdouble Java_java_lang_Double_longBitsToDouble(JNIEnv *env, jclass cls, jlong val) {
    union Double d;
    d.bits = val;

    return d.d;
}
