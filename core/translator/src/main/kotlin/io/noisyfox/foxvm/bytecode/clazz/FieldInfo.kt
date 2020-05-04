package io.noisyfox.foxvm.bytecode.clazz

import io.noisyfox.foxvm.bytecode.isReference
import org.objectweb.asm.Opcodes
import org.objectweb.asm.Type

data class FieldInfo(
    val access: Int,
    val name: String,
    val cIdentifier: String,
    val descriptor: Type,
    val defaultValue: Any?
) {
    val isStatic: Boolean = (access and Opcodes.ACC_STATIC) == Opcodes.ACC_STATIC

    val isReference: Boolean = descriptor.isReference()
}
