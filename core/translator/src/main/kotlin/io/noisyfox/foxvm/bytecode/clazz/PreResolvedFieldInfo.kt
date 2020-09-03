package io.noisyfox.foxvm.bytecode.clazz

data class PreResolvedFieldInfo(
    val field: FieldInfo,
    val fieldIndex: Int,
    val isReference: Boolean
)