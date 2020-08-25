package io.noisyfox.foxvm.bytecode.clazz

import org.objectweb.asm.Opcodes
import org.objectweb.asm.Type
import org.objectweb.asm.tree.MethodNode

data class MethodInfo(
    val declaringClass: ClassInfo,
    val access: Int,
    val name: String,
    val cIdentifier: String,
    val descriptor: Type,
    val signature: String?,
    val methodNode: MethodNode
) {

    val isConstructor: Boolean = INIT == name

    val isClassInitializer: Boolean = CLINIT == name

    val isFinalizer: Boolean = FINALIZE == name

    val isPublic: Boolean = (access and Opcodes.ACC_PUBLIC) == Opcodes.ACC_PUBLIC

    val isPrivate: Boolean = (access and Opcodes.ACC_PRIVATE) == Opcodes.ACC_PRIVATE

    val isStatic: Boolean = (access and Opcodes.ACC_STATIC) == Opcodes.ACC_STATIC

    val isAbstract: Boolean = (access and Opcodes.ACC_ABSTRACT) == Opcodes.ACC_ABSTRACT

    val isNative: Boolean = (access and Opcodes.ACC_NATIVE) == Opcodes.ACC_NATIVE

    val isConcrete: Boolean = !(isAbstract || isNative)

    fun matches(name: String, desc: String): Boolean {
        return this.name == name && descriptor.descriptor == desc
    }

    companion object {
        const val INIT = "<init>"
        const val CLINIT = "<clinit>"
        const val FINALIZE = "finalize"
    }
}
