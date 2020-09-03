package io.noisyfox.foxvm.translator.cgen

import io.noisyfox.foxvm.bytecode.asCString
import io.noisyfox.foxvm.bytecode.clazz.ClassInfo
import io.noisyfox.foxvm.bytecode.clazz.FieldInfo
import io.noisyfox.foxvm.bytecode.clazz.MethodInfo
import io.noisyfox.foxvm.bytecode.clazz.PreResolvedFieldInfo
import io.noisyfox.foxvm.bytecode.resolver.mangleClassName
import org.objectweb.asm.Type
import org.objectweb.asm.tree.LabelNode

fun Int.toCEnum(flagMap: Map<Int, String>): String {
    var f2 = this

    val sb = mutableListOf<String>()
    flagMap.forEach { (f, s) ->
        if (f2 and f == f) {
            sb.add(s)
            f2 = f2 and f.inv()
        }
    }

    assert(f2 == 0) {
        "Unknown flag 0x${"%X".format(f2)}."
    }

    return if (sb.isEmpty()) {
        "0"
    } else {
        sb.joinToString(" | ").intern()
    }
}

/**
 * Translate a boolean value into C BOOLEAN constants
 * `true`  -> "JAVA_TRUE"
 * `false` -> "JAVA_FALSE"
 */
fun Boolean.translate(): String = if (this) {
    "JAVA_TRUE"
} else {
    "JAVA_FALSE"
}

/**
 * C null constant
 * "NULL"
 */
const val CNull = "NULL"

/**
 * C constants for MAX integer literals
 *
 * In C/C++ the integer literals do not take account into the sign, which means
 * -2,147,483,648 and -9,223,372,036,854,775,808 gives you warning like
 * "integer constant is so large that it is unsigned". So we take care of those
 * edge cases by using predefined C constants.
 */
const val C_INT_MIN = "JAVA_INT_MIN"
const val C_INT_MAX = "JAVA_INT_MAX"
const val C_LONG_MIN = "JAVA_LONG_MIN"
const val C_LONG_MAX = "JAVA_LONG_MAX"

/**
 * C constants for NaN and infinity
 */
const val C_FLOAT_NAN = "JAVA_FLOAT_NAN"
const val C_FLOAT_INF = "JAVA_FLOAT_INF"
const val C_FLOAT_NEG_INF = "JAVA_FLOAT_NEG_INF"
const val C_DOUBLE_NAN = "JAVA_DOUBLE_NAN"
const val C_DOUBLE_INF = "JAVA_DOUBLE_INF"
const val C_DOUBLE_NEG_INF = "JAVA_DOUBLE_NEG_INF"

fun Number.toCConst(): String = when (this) {
    is Int -> {
        when(this) {
            Int.MIN_VALUE -> C_INT_MIN
            Int.MAX_VALUE -> C_INT_MAX
            else -> "${this}l"
        }
    }
    is Long -> {
        when(this) {
            Long.MIN_VALUE -> C_LONG_MIN
            Long.MAX_VALUE -> C_LONG_MAX
            else -> "${this}ll"
        }
    }
    is Float -> {
        when {
            isNaN() -> C_FLOAT_NAN
            isInfinite() -> {
                if (this > 0) {
                    C_FLOAT_INF
                } else {
                    C_FLOAT_NEG_INF
                }
            }
            else -> "${this}f"
        }
    }
    is Double -> {
        when {
            isNaN() -> C_DOUBLE_NAN
            isInfinite() -> {
                if (this > 0) {
                    C_DOUBLE_INF
                } else {
                    C_DOUBLE_NEG_INF
                }
            }
            else -> "$this"
        }
    }
    else -> throw IllegalArgumentException("Unsupported type ${this.javaClass}")
}

fun Type.toCBaseTypeName(): String = when (sort) {
    Type.VOID -> "JAVA_VOID"
    Type.BOOLEAN -> "JAVA_BOOLEAN"
    Type.CHAR -> "JAVA_CHAR"
    Type.BYTE -> "JAVA_BYTE"
    Type.SHORT -> "JAVA_SHORT"
    Type.INT -> "JAVA_INT"
    Type.FLOAT -> "JAVA_FLOAT"
    Type.LONG -> "JAVA_LONG"
    Type.DOUBLE -> "JAVA_DOUBLE"
    Type.ARRAY -> "JAVA_ARRAY"
    Type.OBJECT -> "JAVA_OBJECT"
    else -> throw IllegalArgumentException("Unknown type ${sort}.")
}

// See local_transfer_arguments() in vm_stack.h
val Type.localSlotCount: Int
    get() = when(sort) {
        Type.BOOLEAN,
        Type.CHAR,
        Type.BYTE,
        Type.SHORT,
        Type.INT,
        Type.FLOAT,
        Type.ARRAY,
        Type.OBJECT -> 1

        Type.LONG,
        Type.DOUBLE -> 2

        else -> throw IllegalArgumentException("Unknown supported type ${sort}.")
    }

/**
 * A string to "string", or NULL
 */
fun String?.toCString(): String = this?.let { "\"${it.asCString()}\"".intern() } ?: CNull

/**
 * n-dimensions Array of type C where <sig_C> starts with number -> A<n>_<sig_C>
 * n-dimensions Array of type C where <sig_C> not starts with number -> A<n><sig_C>
 *
 * <sig_C> = mangleClassName(C.internalName) if C is object type
 * <sig_C> = one of ZCBSIFJD if C is a primitive type
 */
fun Type.toCSignature(): String = when (sort) {
    Type.VOID -> throw IllegalArgumentException("Void does not have a C signature")
    Type.BOOLEAN -> "Z"
    Type.CHAR -> "C"
    Type.BYTE -> "B"
    Type.SHORT -> "S"
    Type.INT -> "I"
    Type.FLOAT -> "F"
    Type.LONG -> "J"
    Type.DOUBLE -> "D"
    Type.ARRAY -> {
        val d = dimensions
        val elementSignature = elementType.toCSignature()

        if (elementSignature.first() in '0'..'9') {
            "A${d}_$elementSignature"
        } else {
            "A$d$elementSignature"
        }
    }
    Type.OBJECT -> mangleClassName(internalName)
    else -> throw IllegalArgumentException("Unknown type ${sort}.")
}

/**
 * _<sig_arg0>_<sig_arg1>..._R<sig_ret> if has argument and return type is not void
 * _<sig_arg0>_<sig_arg1>...            if has argument and return type is void
 * _R<sig_ret>                          if no argument and return type is not void
 * empty string                         if no argument and return type is void
 *
 * <sig_X> = X.toCSignature()
 */
fun Type.toCMethodSignature(): String {
    assert(sort == Type.METHOD)

    val argumentSig = argumentTypes.joinToString(separator = "") { "_${it.toCSignature()}" }
    val returnSig = returnType.let {
        if (it.sort == Type.VOID) {
            ""
        } else {
            "_R${it.toCSignature()}"
        }
    }

    return argumentSig + returnSig
}

/**
 * The C reference name to the given [ClassInfo]
 */
val ClassInfo.cName: String
    get() = "classInfo${this.cIdentifier}"

/**
 * The C type name of the class created from given [ClassInfo]
 */
val ClassInfo.cClassName: String
    get() = "Class${this.cIdentifier}"

/**
 * The C type name of the object created from given [ClassInfo]
 */
val ClassInfo.cObjectName: String
    get() = "Object${this.cIdentifier}"

/**
 * The C reference name to the given [ClassInfo.superClass]'s [ClassInfo],
 * or [CNull] if this class does not have a super class (only for class java/lang/Object)
 */
val ClassInfo.cSuperClass: String
    get() = superClass?.let {
        "&${it.requireClassInfo().cName}"
    } ?: CNull

/**
 * The C reference name to the given [ClassInfo.interfaces],
 * or [CNull] if this class does not implement any interface
 */
val ClassInfo.cNameInterfaces: String
    get() = if (interfaces.isEmpty()) {
        CNull
    } else {
        "interfaces${cIdentifier}"
    }

/**
 * The C reference name to the given [ClassInfo.fields],
 * or [CNull] if this class does not have any field
 */
val ClassInfo.cNameFields: String
    get() = if (fields.isEmpty()) {
        CNull
    } else {
        "fields${cIdentifier}"
    }

/**
 * The C reference name to the given [ClassInfo.methods],
 * or [CNull] if this class does not have any method
 */
val ClassInfo.cNameMethods: String
    get() = if (methods.isEmpty()) {
        CNull
    } else {
        "methods${cIdentifier}"
    }

/**
 * The C reference name to the given [ClassInfo.preResolvedStaticFields],
 * or [CNull] if this class does not have any static field
 */
val ClassInfo.cNameStaticFields: String
    get() = if (preResolvedStaticFields.isEmpty()) {
        CNull
    } else {
        "fieldsStatic${cIdentifier}"
    }

/**
 * The C reference name to the given [ClassInfo.preResolvedInstanceFields],
 * or [CNull] if this class does not have any instance field
 */
val ClassInfo.cNameInstanceFields: String
    get() = if (preResolvedInstanceFields.isEmpty()) {
        CNull
    } else {
        "fieldsInstance${cIdentifier}"
    }

/**
 * The C reference name to the given [ClassInfo.vtable],
 * or [CNull] if this class does not have any virtual method
 */
val ClassInfo.cNameVTable: String
    get() = if (vtable.isEmpty()) {
        CNull
    } else {
        "vtable${cIdentifier}"
    }

/**
 * The C function name of the class resolve handler.
 */
val ClassInfo.cNameResolveHandler: String
    get() = "resolve_${this.cIdentifier}"

/**
 * The C field name for storing the given field
 */
val PreResolvedFieldInfo.cName: String
    get() = if (this.field.isStatic) {
        "fieldStorage${field.cIdentifier}"
    } else {
        "fieldStorage${field.declaringClass.cIdentifier}${field.cIdentifier}"
    }

/**
 * The C type name for storing the given field
 */
val PreResolvedFieldInfo.cStorageType: String
    get() = field.descriptor.toCBaseTypeName()

/**
 * The C method info structure name of the method.
 */
val MethodInfo.cName: String
    get() {
        return "methodInfo_${declaringClass.cIdentifier}_${this.cIdentifier}${this.descriptor.toCMethodSignature()}"
    }

/**
 * The C function name of the method.
 */
val MethodInfo.cFunctionName: String
    get() {
        return "method_${declaringClass.cIdentifier}_${this.cIdentifier}${this.descriptor.toCMethodSignature()}"
    }

/**
 * The C function declaration of the method.
 */
val MethodInfo.cFunctionDeclaration: String
    get() {
        return "${this.descriptor.returnType.toCBaseTypeName()} ${this.cFunctionName}(VM_PARAM_CURRENT_CONTEXT)"
    }

/**
 * The index of this node.
 */
fun LabelNode.index(info: MethodInfo): Int {
    return info.methodNode.instructions.indexOf(this)
}

/**
 * The C label name of this node.
 */
fun LabelNode.cName(info: MethodInfo): String {
    return "L${index(info)}"
}

val MethodInfo.invokeSuffix: String
    get() = when (val sort = this.descriptor.returnType.sort) {
        Type.VOID -> ""
        Type.BOOLEAN -> "_z"
        Type.CHAR -> "_c"
        Type.BYTE -> "_b"
        Type.SHORT -> "_s"
        Type.INT -> "_i"
        Type.FLOAT -> "_f"
        Type.LONG -> "_l"
        Type.DOUBLE -> "_d"
        Type.ARRAY -> "_a"
        Type.OBJECT -> "_o"
        else -> throw IllegalArgumentException("Unknown type ${sort}.")
    }

val FieldInfo.typeSuffix: String
    get() = when (val sort = this.descriptor.sort) {
        Type.VOID -> throw IllegalArgumentException("Field type cannot be Void")
        Type.BOOLEAN -> "_z"
        Type.CHAR -> "_c"
        Type.BYTE -> "_b"
        Type.SHORT -> "_s"
        Type.INT -> "_i"
        Type.FLOAT -> "_f"
        Type.LONG -> "_l"
        Type.DOUBLE -> "_d"
        Type.ARRAY -> "_a"
        Type.OBJECT -> "_o"
        else -> throw IllegalArgumentException("Unknown type ${sort}.")
    }
