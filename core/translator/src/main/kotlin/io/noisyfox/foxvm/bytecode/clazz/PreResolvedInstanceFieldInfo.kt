package io.noisyfox.foxvm.bytecode.clazz

data class PreResolvedInstanceFieldInfo(
    val declaringClass: Clazz,
    val fieldIndex: Int,
    val isReference: Boolean
)