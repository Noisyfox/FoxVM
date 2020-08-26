package io.noisyfox.foxvm.bytecode.clazz

import org.objectweb.asm.Opcodes
import org.objectweb.asm.Type
import org.objectweb.asm.tree.MethodNode

data class MethodInfo(
    override val declaringClass: ClassInfo,
    override val access: Int,
    override val name: String,
    val cIdentifier: String,
    override val descriptor: Type,
    val signature: String?,
    val methodNode: MethodNode
) : ClassMember {

    val isConstructor: Boolean = INIT == name

    val isClassInitializer: Boolean = CLINIT == name

    val isFinalizer: Boolean = FINALIZE == name

    val isAbstract: Boolean = (access and Opcodes.ACC_ABSTRACT) == Opcodes.ACC_ABSTRACT

    val isNative: Boolean = (access and Opcodes.ACC_NATIVE) == Opcodes.ACC_NATIVE

    val isConcrete: Boolean = !(isAbstract || isNative)

    /** Static, private and constructor methods are not virtual */
    val isVirtual: Boolean = !isPrivate && !isStatic && !isConstructor

    override fun toString(): String {
        return "${declaringClass.thisClass.className}.${name}${descriptor}"
    }

    companion object {
        const val INIT = "<init>"
        const val CLINIT = "<clinit>"
        const val FINALIZE = "finalize"
    }
}
