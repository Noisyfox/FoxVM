package io.noisyfox.foxvm.bytecode.visitor

import io.noisyfox.foxvm.bytecode.clazz.Clazz

interface ClassHandler {
    fun handleRuntimeClass(clazz: Clazz) {
        handleAnyClass(clazz)
    }

    fun handleApplicationClass(clazz: Clazz) {
        handleAnyClass(clazz)
    }

    fun handleAnyClass(clazz: Clazz) {
        throw NotImplementedError("Method must be overridden is ever used.")
    }
}
