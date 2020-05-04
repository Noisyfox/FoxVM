package io.noisyfox.foxvm.translator.cgen

import org.objectweb.asm.Opcodes

/**
 * Must be synced with .\native\runtime\include\vm_base.h
 */
object AccFlag {

    private val classFlags = mapOf(
        Opcodes.ACC_PUBLIC to "CLASS_ACC_PUBLIC",
        Opcodes.ACC_FINAL to "CLASS_ACC_FINAL",
        Opcodes.ACC_SUPER to "CLASS_ACC_SUPER",
        Opcodes.ACC_INTERFACE to "CLASS_ACC_INTERFACE",
        Opcodes.ACC_ABSTRACT to "CLASS_ACC_ABSTRACT",
        Opcodes.ACC_SYNTHETIC to "CLASS_ACC_SYNTHETIC",
        Opcodes.ACC_ANNOTATION to "CLASS_ACC_ANNOTATION",
        Opcodes.ACC_ENUM to "CLASS_ACC_ENUM"
    )
    private val fieldFlags = mapOf(
        Opcodes.ACC_PUBLIC to "FIELD_ACC_PUBLIC",
        Opcodes.ACC_PRIVATE to "FIELD_ACC_PRIVATE",
        Opcodes.ACC_PROTECTED to "FIELD_ACC_PROTECTED",
        Opcodes.ACC_STATIC to "FIELD_ACC_STATIC",
        Opcodes.ACC_FINAL to "FIELD_ACC_FINAL",
        Opcodes.ACC_VOLATILE to "FIELD_ACC_VOLATILE",
        Opcodes.ACC_TRANSIENT to "FIELD_ACC_TRANSIENT",
        Opcodes.ACC_SYNTHETIC to "FIELD_ACC_SYNTHETIC",
        Opcodes.ACC_ENUM to "FIELD_ACC_ENUM"
    )
    private val methodFlags = mapOf(
        Opcodes.ACC_PUBLIC to "METHOD_ACC_PUBLIC",
        Opcodes.ACC_PRIVATE to "METHOD_ACC_PRIVATE",
        Opcodes.ACC_PROTECTED to "METHOD_ACC_PROTECTED",
        Opcodes.ACC_STATIC to "METHOD_ACC_STATIC",
        Opcodes.ACC_FINAL to "METHOD_ACC_FINAL",
        Opcodes.ACC_SYNCHRONIZED to "METHOD_ACC_SYNCHRONIZED",
        Opcodes.ACC_BRIDGE to "METHOD_ACC_BRIDGE",
        Opcodes.ACC_VARARGS to "METHOD_ACC_VARARGS",
        Opcodes.ACC_NATIVE to "METHOD_ACC_NATIVE",
        Opcodes.ACC_ABSTRACT to "METHOD_ACC_ABSTRACT",
        Opcodes.ACC_STRICT to "METHOD_ACC_STRICTFP",
        Opcodes.ACC_SYNTHETIC to "METHOD_ACC_SYNTHETIC"
    )

    private val classCache = mutableMapOf<Int, String>()
    private val fieldCache = mutableMapOf<Int, String>()
    private val methodCache = mutableMapOf<Int, String>()

    /**
     * Translate the given acc flag into C enum combinations.
     * 0x0011 -> CLASS_ACC_PUBLIC | CLASS_ACC_FINAL
     */
    fun translateClassAcc(flag: Int): String = classCache.getOrPut(flag) {
        flag.toCEnum(classFlags)
    }

    /**
     * Translate the given acc flag into C enum combinations.
     * 0x0011 -> FIELD_ACC_PUBLIC | FIELD_ACC_FINAL
     */
    fun translateFieldAcc(flag: Int): String = fieldCache.getOrPut(flag) {
        flag.toCEnum(fieldFlags)
    }

    /**
     * Translate the given acc flag into C enum combinations.
     * 0x0011 -> METHOD_ACC_PUBLIC | METHOD_ACC_FINAL
     */
    fun translateMethodAcc(flag: Int): String = methodCache.getOrPut(flag) {
        flag.toCEnum(methodFlags)
    }
}
