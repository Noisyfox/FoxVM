package io.noisyfox.foxvm.bytecode.clazz

data class ClassInfo(
    val version: Int,
    val superClass: Clazz?,
    val interfaces: List<Clazz>,
    val fields: MutableList<FieldInfo> = mutableListOf(),
    // TODO: methods
    val preResolvedStaticFields: MutableList<PreResolvedStaticFieldInfo> = mutableListOf(),
    val preResolvedInstanceFields: MutableList<PreResolvedInstanceFieldInfo> = mutableListOf()
)
