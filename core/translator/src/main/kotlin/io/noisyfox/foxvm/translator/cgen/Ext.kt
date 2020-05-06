package io.noisyfox.foxvm.translator.cgen

import io.noisyfox.foxvm.bytecode.clazz.ClassInfo
import io.noisyfox.foxvm.bytecode.clazz.PreResolvedInstanceFieldInfo
import io.noisyfox.foxvm.bytecode.clazz.PreResolvedStaticFieldInfo
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
fun PreResolvedInstanceFieldInfo.cNameEnum(): String {
    val info = requireNotNull(declaringClass.classInfo)
    val field = info.fields[this.fieldIndex]

    return "FIELD_INSTANCE_${info.cIdentifier}${field.cIdentifier}"
}

/**
 * The C field name for storing the given instance field
 */
fun PreResolvedInstanceFieldInfo.cName(): String {
    val info = requireNotNull(declaringClass.classInfo)
    val field = info.fields[this.fieldIndex]

    return "fieldStorage${info.cIdentifier}${field.cIdentifier}"
}

/**
 * The C type name for storing the given instance field
 */
fun PreResolvedInstanceFieldInfo.cStorageType(): String {
    val info = requireNotNull(declaringClass.classInfo)
    val field = info.fields[this.fieldIndex]

    return field.descriptor.toCBaseTypeName()
}
