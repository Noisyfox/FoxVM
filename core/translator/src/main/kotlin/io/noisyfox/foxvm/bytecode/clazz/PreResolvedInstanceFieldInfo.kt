package io.noisyfox.foxvm.bytecode.clazz

data class PreResolvedInstanceFieldInfo(
    val field: FieldInfo,
    val fieldIndex: Int,
    val isReference: Boolean
)