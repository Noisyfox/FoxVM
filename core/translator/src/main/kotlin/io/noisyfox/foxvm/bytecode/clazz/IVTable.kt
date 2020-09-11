package io.noisyfox.foxvm.bytecode.clazz

data class IVTableItem(
    val declaringInterface: ClassInfo,
    val methodIndexes: List<IVTableMethodIndex>
)

data class IVTableMethodIndex(
    val methodIndex: Int,
    val vtableIndex: Int
)
