//
// Created by noisyfox on 2020/9/3.
//

#include "vm_base.h"
#include "vm_thread.h"
#include "vm_bytecode.h"
#include "vm_array.h"
#include <string.h>

JNIEXPORT void JNICALL Java_java_lang_System_arraycopyPrimitive(VM_PARAM_CURRENT_CONTEXT, jclass cls, jarray src, jint srcPos,
                                                                jarray dest, jint destPos, jint length) {
    native_exit_jni(vmCurrentContext);

    JAVA_ARRAY srcObj = (JAVA_ARRAY) native_dereference(vmCurrentContext, src);
    JAVA_ARRAY destObj = (JAVA_ARRAY) native_dereference(vmCurrentContext, dest);

    BasicType componentType = array_type_of(obj_get_class(&srcObj->baseObject)->info->thisClass);
    assert(is_java_primitive(componentType));
    void *srcPtr = array_element_at(srcObj, componentType, srcPos);
    void *destPtr = array_element_at(destObj, componentType, destPos);
    memmove(destPtr, srcPtr, length * type_size(componentType));

    native_enter_jni(vmCurrentContext);
}

/*
 * The following three functions implement setter methods for
 * java.lang.System.{in, out, err}. They are natively implemented
 * because they violate the semantics of the language (i.e. set final
 * variable).
 */
JNIEXPORT void JNICALL
Java_java_lang_System_setIn0(JNIEnv *env, jclass cla, jobject stream)
{
    jfieldID fid =
            (*env)->GetStaticFieldID(env,cla,"in","Ljava/io/InputStream;");
    if (fid == 0)
        return;
    (*env)->SetStaticObjectField(env,cla,fid,stream);
}

JNIEXPORT void JNICALL
Java_java_lang_System_setOut0(JNIEnv *env, jclass cla, jobject stream)
{
    jfieldID fid =
            (*env)->GetStaticFieldID(env,cla,"out","Ljava/io/PrintStream;");
    if (fid == 0)
        return;
    (*env)->SetStaticObjectField(env,cla,fid,stream);
}

JNIEXPORT void JNICALL
Java_java_lang_System_setErr0(JNIEnv *env, jclass cla, jobject stream)
{
    jfieldID fid =
            (*env)->GetStaticFieldID(env,cla,"err","Ljava/io/PrintStream;");
    if (fid == 0)
        return;
    (*env)->SetStaticObjectField(env,cla,fid,stream);
}
