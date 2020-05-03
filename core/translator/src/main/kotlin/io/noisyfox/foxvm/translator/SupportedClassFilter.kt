package io.noisyfox.foxvm.translator

import io.noisyfox.foxvm.bytecode.clazz.Clazz
import io.noisyfox.foxvm.bytecode.visitor.ClassHandler

class SupportedClassFilter(
    private val classHandler: ClassHandler
) : ClassHandler {

    private fun isClassSupported(clazz: Clazz): Boolean {
        if ("module-info" == clazz.className) {
            return false
        }

        return true
    }

    override fun handleAnyClass(clazz: Clazz) {
        if (isClassSupported(clazz)) {
            clazz.accept(classHandler)
        }
    }
}