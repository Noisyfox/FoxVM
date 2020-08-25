package io.noisyfox.foxvm.bytecode.clazz

import io.noisyfox.foxvm.bytecode.isReference
import org.objectweb.asm.Opcodes
import org.objectweb.asm.Type

data class FieldInfo(
    val declaringClass: ClassInfo,
    val access: Int,
    val name: String,
    val cIdentifier: String,
    val descriptor: Type,
    val signature: String?,
    val defaultValue: Any?
) {
    val isStatic: Boolean = (access and Opcodes.ACC_STATIC) == Opcodes.ACC_STATIC

    val isReference: Boolean = descriptor.isReference()

    /**
     * jvms8 $5.4.3.2 Field Resolution states:
     * > If C declares a field with the name and descriptor specified by the field
     * > reference, field lookup succeeds.
     *
     * Here we check if the name and descriptor matches.
     */
    fun matches(name: String, desc: String): Boolean {
        return this.name == name && descriptor.descriptor == desc
    }
}
