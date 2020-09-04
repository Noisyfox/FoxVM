package io.noisyfox.foxvm.translator.cgen

import io.noisyfox.foxvm.bytecode.clazz.MethodInfo
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
}

class JNIMethod(
    private val info: MethodInfo
) {
    val isStatic: Boolean = info.isStatic
    val returnType: Type = info.descriptor.returnType

    private val argumentTypes: List<Type> = info.descriptor.argumentTypes.toList()

    val argumentCount = argumentTypes.size.let {
        if (isStatic) {
            it
        } else {
            it + 1 // One extra slot for `this`
        }
    }

    // For native method, `maxStack` and `maxLocals` are always 0
    // here we use the parameter types to determine the `maxLocals` so we can pass parameters
    // like a normal method.
    val localSlotCount: Int = argumentTypes.fold(0) { acc, type -> acc + type.localSlotCount }
        .let {
            if (isStatic) {
                it
            } else {
                it + 1 // One extra slot for `this`
            }
        }

    fun functionPtrDecl(variableName: String): String = declFunctionPtr(variableName, isStatic, returnType, argumentTypes)

    val referenceArgumentsPreparationCode: String
        get() {
            val sb = StringBuilder()
            if (isStatic) {
                // Get the reference handler for this class
                sb.append(
                    """
                |    jclass argThisClazz = bc_jni_arg_this_class();
                |""".trimMargin()
                )
            }

            val arguments: List<Type> = if (isStatic) {
                argumentTypes
            } else {
                listOf(Type.getObjectType(info.declaringClass.thisClass.className)) + argumentTypes
            }
            var localIndex = 0
            arguments.forEach {
                if (it.sort == Type.OBJECT || it.sort == Type.ARRAY) {
                    sb.append(
                        """
                |    ${it.toJNIType()} argRef_${localIndex} = bc_jni_arg_jref($localIndex, ${it.toJNIType()});
                |""".trimMargin()
                    )
                }

                localIndex += it.localSlotCount
            }

            return sb.toString()
        }

    val argumentsPassingCode: String
        get() {
            // Build argument list
            val jniArgs = mutableListOf<String>()
            // Pass JNIEnv
            jniArgs.add("&vmCurrentContext->jni")

            if (isStatic) {
                // Pass current class
                jniArgs.add("argThisClazz")
            }

            // Pass remaining arguments
            val arguments: List<Type> = if (isStatic) {
                argumentTypes
            } else {
                listOf(Type.getObjectType(info.declaringClass.thisClass.className)) + argumentTypes
            }
            var localIndex = 0
            arguments.mapTo(jniArgs) { arg ->
                when (arg.sort) {
                    Type.BOOLEAN,
                    Type.CHAR,
                    Type.BYTE,
                    Type.SHORT,
                    Type.INT,
                    Type.FLOAT,
                    Type.LONG,
                    Type.DOUBLE -> "bc_jni_arg_${arg.toJNIType()}($localIndex)"

                    Type.ARRAY,
                    Type.OBJECT -> "argRef_${localIndex}"

                    else -> throw IllegalArgumentException("Unexpected type ${arg.sort} for argument.")
                }.also {
                    localIndex += arg.localSlotCount
                }
            }

            return jniArgs.joinToString(
                separator = """
                |,
                |        """.trimMargin()
            )
        }

    companion object {
        private fun declFunctionPtr(variableName: String, isStatic: Boolean, returnType: Type, arguments: List<Type>): String {
            val sb = StringBuilder()
            sb.append(returnType.toJNIType())
            sb.append(" (* JNICALL $variableName)(JNIEnv *, ")

            if (isStatic) {
                // The second argument to a static native method is a reference to its Java class.
                sb.append(Jni.TYPE_CLASS)
            } else {
                // The second argument to a nonstatic native method is a reference to the object.
                sb.append(Jni.TYPE_OBJECT)
            }

            // remaining arguments
            arguments.joinToString(separator = "") { ", ${it.toJNIType()}" }
                .let { sb.append(it) }

            sb.append(")")

            return sb.toString().intern()
        }
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

// https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/design.html
// Resolving Native Method Names
// Dynamic linkers resolve entries based on their names. A native method name is concatenated
// from the following components:
// • the prefix Java_
// • a mangled fully-qualified class name
// • an underscore (“_”) separator
// • a mangled method name
// • for overloaded native methods, two underscores (“__”) followed by the mangled argument signature
// The VM checks for a method name match for methods that reside in the native library. The VM looks
// first for the short name; that is, the name without the argument signature. It then looks for the
// long name, which is the name with the argument signature.
val MethodInfo.jniShortName: String
    get() = "Java_${this.declaringClass.thisClass.className.asJNIIdentifier()}_${this.name.asJNIIdentifier()}"

val MethodInfo.jniLongName: String
    get() {
        val desc = this.descriptor.descriptor
        // Keep the argument desc only
        val argDesc = desc.substring(1, desc.lastIndexOf(')'))
        return "${this.jniShortName}__${argDesc.asJNIIdentifier()}"
    }


// We adopted a simple name-mangling scheme to ensure that all Unicode characters translate into valid
// C function names. We use the underscore (“_”) character as the substitute for the slash (“/”) in fully
// qualified class names. Since a name or type descriptor never begins with a number, we can use
// _0, ..., _9 for escape sequences, as the following table illustrates:
//
// Unicode Character Translation
// +-----------------+----------------------
// | Escape Sequence | Denotes
// +-----------------+----------------------
// | _0XXXX          | a Unicode character XXXX. Note that lower case is used to represent non-ASCII Unicode
// |                 | characters, for example, _0abcd as opposed to _0ABCD.
// +-----------------+----------------------
// | _1              | the character “_”
// +-----------------+----------------------
// | _2              | the character “;” in signatures
// +-----------------+----------------------
// | _3              | the character “[“ in signatures
// +-----------------+----------------------
private fun Char.asJNIIdentifier(): String = when (this) {
    in '0'..'9', in 'a'..'z', in 'A'..'Z' -> this.toString()
    '/' -> "_"
    '_' -> "_1"
    ';' -> "_2"
    '[' -> "_3"
    else -> "_0%04x".format(this.toInt())
}

private fun String.asJNIIdentifier(): String = asSequence().joinToString(separator = "") { it.asJNIIdentifier() }.intern()
