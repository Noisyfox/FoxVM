package io.noisyfox.foxvm.bytecode.visitor

import io.noisyfox.foxvm.bytecode.ClassPool
import io.noisyfox.foxvm.bytecode.clazz.Clazz

class ClassPresenceFilter(
    private val classPool: ClassPool,
    private val handlerWhenPresented: ClassHandler? = null,
    private val handlerWhenMissing: ClassHandler? = null
) : ClassHandler {
    override fun handleAnyClass(clazz: Clazz) {
        if (classPool.getClass(clazz.className) != null) {
            handlerWhenPresented
        } else {
            handlerWhenMissing
        }?.let {
            clazz.accept(it)
        }
    }
}
