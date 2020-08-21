package io.noisyfox.foxvm.bytecode.visitor

interface ClassPoolVisitor : ClassHandler {
    fun visitStart() {}

    fun visitEnd() {}
}