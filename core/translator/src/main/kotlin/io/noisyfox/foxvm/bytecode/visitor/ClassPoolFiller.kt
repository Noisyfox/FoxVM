package io.noisyfox.foxvm.bytecode.visitor

import io.noisyfox.foxvm.bytecode.ClassPool
import io.noisyfox.foxvm.bytecode.clazz.Clazz

class ClassPoolFiller(
    private val classPool: ClassPool
) : ClassHandler {
    override fun handleAnyClass(clazz: Clazz) {
        classPool.addClass(clazz)
    }
}
