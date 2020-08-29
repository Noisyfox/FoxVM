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

    // jvms8 §2.9 Special Methods
    // A class or interface has at most one class or interface initialization method and is
    // initialized (§5.5) by invoking that method. The initialization method of a class or
    // interface has the special name <clinit>, takes no arguments, and is void (§4.3.3).
    val isClassInitializer: Boolean = CLINIT == name
        && descriptor.returnType == Type.VOID_TYPE
        && descriptor.argumentTypes.isEmpty()
        // In a class file whose version number is 51.0 or above, the method must
        // additionally have its ACC_STATIC flag (§4.6) set in order to be the class or interface
        // initialization method.
        // This requirement was introduced in Java SE 7. In a class file whose version number is 50.0
        // or below, a method named <clinit> that is void and takes no arguments is considered the
        // class or interface initialization method regardless of the setting of its ACC_STATIC flag.
        && (declaringClass.version <= 50 || isStatic)

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
