package io.noisyfox.foxvm.bytecode.clazz

import org.objectweb.asm.Opcodes
import org.objectweb.asm.Type

data class MethodInfo(
    val access: Int,
    val name: String,
    val cIdentifier: String,
    val descriptor: Type
) {

    val isConstructor: Boolean = INIT == name

    val isClassInitializer: Boolean = CLINIT == name

    val isFinalizer: Boolean = FINALIZE == name

    val isStatic: Boolean = (access and Opcodes.ACC_STATIC) == Opcodes.ACC_STATIC

    companion object {
        const val INIT = "<init>"
        const val CLINIT = "<clinit>"
        const val FINALIZE = "finalize"
    }
}
