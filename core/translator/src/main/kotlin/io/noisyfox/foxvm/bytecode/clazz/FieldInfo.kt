package io.noisyfox.foxvm.bytecode.clazz

import io.noisyfox.foxvm.bytecode.isReference
import org.objectweb.asm.Type

data class FieldInfo(
    override val declaringClass: ClassInfo,
    override val access: Int,
    override val name: String,
    val cIdentifier: String,
    override val descriptor: Type,
    val signature: String?,
    val defaultValue: Any?
) : ClassMember {
    val isReference: Boolean = descriptor.isReference()

    override fun toString(): String {
        return "${declaringClass.thisClass.className}.${name}:${descriptor}"
    }
}
