package io.noisyfox.foxvm.bytecode.clazz

import org.objectweb.asm.Opcodes

data class ClassInfo(
    val thisClass: Clazz,
    val cIdentifier: String,
    val version: Int,
    val signature: String?,
    val superClass: Clazz?,
    val interfaces: List<Clazz>,
    val fields: MutableList<FieldInfo> = mutableListOf(),
    val methods: MutableList<MethodInfo> = mutableListOf(),
    val preResolvedStaticFields: MutableList<PreResolvedStaticFieldInfo> = mutableListOf(),
    val preResolvedInstanceFields: MutableList<PreResolvedInstanceFieldInfo> = mutableListOf()
) {

    val isEnum: Boolean = (thisClass.access and Opcodes.ACC_ENUM) == Opcodes.ACC_ENUM

    /** The finalizer declared by this class */
    val declFinalizer: MethodInfo?
        get() = when {
            isEnum -> {
                // Enums do not have finalizer
                null
            }
            thisClass.className in UnfinalizableClasses -> {
                null
            }
            else -> {
                methods.firstOrNull { it.isFinalizer }
            }
        }

    /** The finalizer declared by this class or inherited from [superClass] */
    val finalizer: MethodInfo?
        get() = declFinalizer ?: superClass?.requireClassInfo()?.finalizer

    private companion object {
        private val UnfinalizableClasses = setOf(
            Clazz.CLASS_JAVA_LANG_OBJECT,
            Clazz.CLASS_JAVA_LANG_ENUM
        )
    }
}
