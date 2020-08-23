package io.noisyfox.foxvm.bytecode.clazz

import org.objectweb.asm.Opcodes
import org.objectweb.asm.tree.FieldInsnNode

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

    /**
     * Look up the field referenced by [inst] in current class.
     *
     * See jvms8 §5.4.3.2 Field Resolution for more details
     *
     * @return the [FieldInfo] if the referenced field is found, `null` otherwise.
     */
    fun fieldLookup(inst: FieldInsnNode): FieldInfo? {
        // 1. If C declares a field with the name and descriptor specified by the field
        //    reference, field lookup succeeds. The declared field is the result of the field
        //    lookup.
        val decls = this.fields.filter { it.matches(inst) }
        when (decls.size) {
            0 -> {
            }
            1 -> return decls.single()
            else -> throw ClassFormatError("Multiple fields with same name and descriptor found in class ${this.thisClass.className}: $decls")
        }

        // 2. Otherwise, field lookup is applied recursively to the direct superinterfaces of
        //    the specified class or interface C.
        this.interfaces.forEach {
            val f = it.requireClassInfo().fieldLookup(inst)
            if (f != null) {
                return f
            }
        }

        // 3. Otherwise, if C has a superclass S, field lookup is applied recursively to S.
        val f = this.superClass?.requireClassInfo()?.fieldLookup(inst)
        if (f != null) {
            return f
        }

        // 4. Otherwise, field lookup fails.
        return null
    }

    override fun toString(): String {
        return "${ClassInfo::class.simpleName} [${thisClass.className}]"
    }

    private companion object {
        private val UnfinalizableClasses = setOf(
            Clazz.CLASS_JAVA_LANG_OBJECT,
            Clazz.CLASS_JAVA_LANG_ENUM
        )
    }
}
