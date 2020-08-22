package io.noisyfox.foxvm.bytecode.clazz

data class PreResolvedStaticFieldInfo(
    val field: FieldInfo,
    val fieldIndex: Int,
    val isReference: Boolean
)
