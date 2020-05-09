package io.noisyfox.foxvm.translator.cgen

import io.noisyfox.foxvm.bytecode.clazz.ClassInfo
import io.noisyfox.foxvm.bytecode.clazz.MethodInfo
import io.noisyfox.foxvm.bytecode.clazz.PreResolvedInstanceFieldInfo
import io.noisyfox.foxvm.bytecode.clazz.PreResolvedStaticFieldInfo
import io.noisyfox.foxvm.bytecode.resolver.mangleClassName
import org.objectweb.asm.Type

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
        it.classInfo!!.cName
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
 * The C function name of the class resolve handler.
 */
val ClassInfo.cNameResolveHandler: String
    get() = "resolve_${this.cIdentifier}"

/**
 * The C function name of the finalizer, either from superclass, or declared by this class,
 * or [CNull] if none of the super classes or this class itself declare a finalizer.
 */
val ClassInfo.cNameFinalizer: String
    get() {
        return finalizer?.cName(this) // Check if this class declares a finalizer
            ?: superClass?.requireClassInfo()?.cNameFinalizer // Check super class
            ?: CNull
    }

/**
 * The C enum name for referencing the given static field
 */
fun PreResolvedStaticFieldInfo.cNameEnum(info: ClassInfo): String {
    val field = info.fields[this.fieldIndex]

    return "FIELD_STATIC_${info.cIdentifier}${field.cIdentifier}"
}

/**
 * The C field name for storing the given static field
 */
fun PreResolvedStaticFieldInfo.cName(info: ClassInfo): String {
    val field = info.fields[this.fieldIndex]

    return "fieldStorage${field.cIdentifier}"
}

/**
 * The C type name for storing the given static field
 */
fun PreResolvedStaticFieldInfo.cStorageType(info: ClassInfo): String {
    val field = info.fields[this.fieldIndex]

    return field.descriptor.toCBaseTypeName()
}

/**
 * The C enum name for referencing the given instance field
 */
val PreResolvedInstanceFieldInfo.cNameEnum: String
    get() {
        val info = declaringClass.requireClassInfo()
        val field = info.fields[this.fieldIndex]

        return "FIELD_INSTANCE_${info.cIdentifier}${field.cIdentifier}"
    }

/**
 * The C field name for storing the given instance field
 */
val PreResolvedInstanceFieldInfo.cName: String
    get() {
        val info = declaringClass.requireClassInfo()
        val field = info.fields[this.fieldIndex]

        return "fieldStorage${info.cIdentifier}${field.cIdentifier}"
    }

/**
 * The C type name for storing the given instance field
 */
val PreResolvedInstanceFieldInfo.cStorageType: String
    get() {
        val info = declaringClass.requireClassInfo()
        val field = info.fields[this.fieldIndex]

        return field.descriptor.toCBaseTypeName()
    }

/**
 * The C function name of the method.
 */
fun MethodInfo.cName(info: ClassInfo): String {
    return "method_${info.cIdentifier}_${this.cIdentifier}${this.descriptor.toCMethodSignature()}"
}

/**
 * The C function declaration of the method.
 */
fun MethodInfo.cDeclaration(info: ClassInfo): String {
    return "${this.descriptor.returnType.toCBaseTypeName()} ${this.cName(info)}(VM_PARAM_CURRENT_CONTEXT)"
}
