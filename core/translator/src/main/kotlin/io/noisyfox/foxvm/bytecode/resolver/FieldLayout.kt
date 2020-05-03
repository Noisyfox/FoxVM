package io.noisyfox.foxvm.bytecode.resolver

import org.objectweb.asm.Type

object FieldLayout : Comparator<Type> {

    /**
     * Object fields organized in memory in the following order:
     *
     * 1. doubles and longs
     * 2. ints and floats
     * 3. shorts and chars
     * 4. booleans and bytes
     * 5. references
     */
    private fun Type.getOrder(): Int = when (sort) {
        Type.DOUBLE, Type.LONG -> 0
        Type.INT, Type.FLOAT -> 1
        Type.SHORT, Type.CHAR -> 2
        Type.BOOLEAN, Type.BYTE -> 3
        Type.OBJECT, Type.ARRAY -> 4
        else -> throw IllegalArgumentException("Unsupported type $this")
    }

    override fun compare(t1: Type, t2: Type): Int {
        return t1.getOrder().compareTo(t2.getOrder())
    }
}
