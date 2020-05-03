package io.noisyfox.foxvm.bytecode

import io.noisyfox.foxvm.bytecode.clazz.Clazz
import io.noisyfox.foxvm.bytecode.visitor.ClassHandler
import java.util.TreeMap

interface ClassPool {

    fun addClass(clazz: Clazz)

    fun removeClass(clazz: Clazz) {
        removeClass(clazz.className)
    }

    fun removeClass(className: String)

    fun getClass(className: String): Clazz?

    fun accept(classHandler: ClassHandler)
}

class SimpleClassPool : ClassPool {
    private val classes: TreeMap<String, Clazz> = TreeMap()

    override fun addClass(clazz: Clazz) {
        classes[clazz.className] = clazz
    }

    override fun removeClass(className: String) {
        classes.remove(className)
    }

    override fun getClass(className: String): Clazz? = classes[className]

    override fun accept(classHandler: ClassHandler) {
        classes.forEach { (_, clazz) ->
            clazz.accept(classHandler)
        }
    }
}

class CombinedClassPool(
    private val runtimeClassPool: ClassPool,
    private val applicationClassPool: ClassPool
) : ClassPool {
    override fun addClass(clazz: Clazz) {
        if (clazz.isRuntimeClass) {
            runtimeClassPool.addClass(clazz)
        } else {
            applicationClassPool.addClass(clazz)
        }
    }

    override fun removeClass(className: String) {
        applicationClassPool.removeClass(className)
        runtimeClassPool.removeClass(className)
    }

    override fun getClass(className: String): Clazz? {
        return applicationClassPool.getClass(className) ?: runtimeClassPool.getClass(className)
    }

    override fun accept(classHandler: ClassHandler) {
        applicationClassPool.accept(classHandler)
        runtimeClassPool.accept(classHandler)
    }
}
