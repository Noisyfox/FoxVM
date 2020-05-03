package io.noisyfox.foxvm.bytecode

import org.objectweb.asm.Type

fun Type.isReference() = when (this.sort) {
    Type.OBJECT, Type.ARRAY -> true
    else -> false
}
