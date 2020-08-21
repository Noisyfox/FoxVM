package io.noisyfox.foxvm.bytecode

import io.noisyfox.foxvm.bytecode.clazz.Clazz
import io.noisyfox.foxvm.bytecode.visitor.ClassHandler
import io.noisyfox.foxvm.bytecode.visitor.ClassPoolVisitor
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

abstract class AbstractClassPool : ClassPool {

    final override fun accept(classHandler: ClassHandler) {
        if (classHandler is ClassPoolVisitor) {
            classHandler.visitStart()
        }

        visitClasses(classHandler)

        if (classHandler is ClassPoolVisitor) {
            classHandler.visitEnd()
        }
    }

    abstract fun visitClasses(classHandler: ClassHandler)
}

class SimpleClassPool : AbstractClassPool() {
    private val classes: TreeMap<String, Clazz> = TreeMap()

    override fun addClass(clazz: Clazz) {
        classes[clazz.className] = clazz
    }

    override fun removeClass(className: String) {
        classes.remove(className)
    }

    override fun getClass(className: String): Clazz? = classes[className]

    override fun visitClasses(classHandler: ClassHandler) {
        classes.forEach { (_, clazz) ->
            clazz.accept(classHandler)
        }
    }
}

class CombinedClassPool(
    private val runtimeClassPool: ClassPool,
    private val applicationClassPool: ClassPool
) : AbstractClassPool() {
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

    override fun visitClasses(classHandler: ClassHandler) {
        if (applicationClassPool is AbstractClassPool) {
            applicationClassPool.visitClasses(classHandler)
        } else {
            applicationClassPool.accept(classHandler)
        }
        if (runtimeClassPool is AbstractClassPool) {
            runtimeClassPool.visitClasses(classHandler)
        } else {
            runtimeClassPool.accept(classHandler)
        }
    }
}
