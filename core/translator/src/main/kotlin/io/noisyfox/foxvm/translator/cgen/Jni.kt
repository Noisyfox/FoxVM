package io.noisyfox.foxvm.translator.cgen

import org.objectweb.asm.Type

object Jni {
    // Primitive types
    const val TYPE_BOOLEAN = "jboolean"
    const val TYPE_BYTE = "jbyte"
    const val TYPE_CHAR = "jchar"
    const val TYPE_SHORT = "jshort"
    const val TYPE_INT = "jint"
    const val TYPE_LONG = "jlong"
    const val TYPE_FLOAT = "jfloat"
    const val TYPE_DOUBLE = "jdouble"
    const val TYPE_VOID = "void"
    const val TYPE_SIZE = "jsize"

    // Reference types
    const val TYPE_OBJECT = "jobject"
    const val TYPE_CLASS = "jclass"
    const val TYPE_STRING = "jstring"
    const val TYPE_THROWABLE = "jthrowable"

    // Array types
    const val TYPE_ARRAY = "jarray"
    const val TYPE_OBJECT_ARRAY = "jobjectArray"
    const val TYPE_BOOLEAN_ARRAY = "jbooleanArray"
    const val TYPE_BYTE_ARRAY = "jbyteArray"
    const val TYPE_CHAR_ARRAY = "jcharArray"
    const val TYPE_SHORT_ARRAY = "jshortArray"
    const val TYPE_INT_ARRAY = "jintArray"
    const val TYPE_LONG_ARRAY = "jlongArray"
    const val TYPE_FLOAT_ARRAY = "jfloatArray"
    const val TYPE_DOUBLE_ARRAY = "jdoubleArray"

    // Field and method IDs
    const val TYPE_FIELD_ID = "jfieldID"
    const val TYPE_METHOD_ID = "jmethodID"

    fun declFunctionPtr(variableName: String, isStatic: Boolean, returnType: Type, arguments: List<Type>): String {
        val sb = StringBuilder()
        sb.append(returnType.toJNIType())
        sb.append(" (* $variableName)(JNIEnv *, ")

        if (isStatic) {
            // The second argument to a static native method is a reference to its Java class.
            sb.append(TYPE_CLASS)
        } else {
            // The second argument to a nonstatic native method is a reference to the object.
            sb.append(TYPE_OBJECT)
        }

        // remaining arguments
        arguments.joinToString(separator = "") { ", ${it.toJNIType()}" }
            .let { sb.append(it) }

        sb.append(")")

        return sb.toString().intern()
    }
}

fun Type.toJNIType(): String = when (sort) {
    Type.VOID -> Jni.TYPE_VOID
    Type.BOOLEAN -> Jni.TYPE_BOOLEAN
    Type.CHAR -> Jni.TYPE_CHAR
    Type.BYTE -> Jni.TYPE_BYTE
    Type.SHORT -> Jni.TYPE_SHORT
    Type.INT -> Jni.TYPE_INT
    Type.FLOAT -> Jni.TYPE_FLOAT
    Type.LONG -> Jni.TYPE_LONG
    Type.DOUBLE -> Jni.TYPE_DOUBLE

    Type.ARRAY -> {
        if (this.dimensions > 1) {
            Jni.TYPE_OBJECT_ARRAY
        } else {
            when (elementType.sort) {
                Type.BOOLEAN -> Jni.TYPE_BOOLEAN_ARRAY
                Type.CHAR -> Jni.TYPE_CHAR_ARRAY
                Type.BYTE -> Jni.TYPE_BYTE_ARRAY
                Type.SHORT -> Jni.TYPE_SHORT_ARRAY
                Type.INT -> Jni.TYPE_INT_ARRAY
                Type.FLOAT -> Jni.TYPE_FLOAT_ARRAY
                Type.LONG -> Jni.TYPE_LONG_ARRAY
                Type.DOUBLE -> Jni.TYPE_DOUBLE_ARRAY
                Type.OBJECT -> Jni.TYPE_OBJECT_ARRAY
                else -> Jni.TYPE_ARRAY
            }
        }
    }

    Type.OBJECT -> {
        when (descriptor) {
            "Ljava/lang/String;" -> Jni.TYPE_STRING
            "Ljava/lang/Class;" -> Jni.TYPE_CLASS
            else -> Jni.TYPE_OBJECT
        }
    }

    else -> throw IllegalArgumentException("Unknown type ${sort}.")
}
