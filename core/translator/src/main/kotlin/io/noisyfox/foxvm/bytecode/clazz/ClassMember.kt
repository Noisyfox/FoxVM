package io.noisyfox.foxvm.bytecode.clazz

import org.objectweb.asm.Opcodes
import org.objectweb.asm.Type

interface ClassMember {
    val declaringClass: ClassInfo
    val access: Int
    val name: String
    val descriptor: Type

    val isPublic: Boolean
        get() = (access and Opcodes.ACC_PUBLIC) == Opcodes.ACC_PUBLIC

    val isPrivate: Boolean
        get() = (access and Opcodes.ACC_PRIVATE) == Opcodes.ACC_PRIVATE

    val isProtected: Boolean
        get() = (access and Opcodes.ACC_PROTECTED) == Opcodes.ACC_PROTECTED

    val isStatic: Boolean
        get() = (access and Opcodes.ACC_STATIC) == Opcodes.ACC_STATIC

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
