package io.noisyfox.foxvm.bytecode.clazz

data class PreResolvedStaticFieldInfo(
    val fieldIndex: Int,
    val isReference: Boolean
)
