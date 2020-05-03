package io.noisyfox.foxvm.translator

import io.noisyfox.foxvm.bytecode.ClassPool
import io.noisyfox.foxvm.bytecode.clazz.Clazz
import io.noisyfox.foxvm.bytecode.visitor.ClassHandler
import org.slf4j.LoggerFactory

class DuplicateClassPrinter(
    private val classPool: ClassPool
) : ClassHandler {

    private fun getExistingClassPath(clazz: Clazz): String {
        val name = clazz.className

        return classPool.getClass(name)?.filePath ?: "<Unknown>"
    }

    override fun handleRuntimeClass(clazz: Clazz) {
        LOGGER.warn(
            "Duplicated runtime class definition found: [{}] at {}, previously defined at {}",
            clazz.className,
            clazz.filePath,
            getExistingClassPath(clazz)
        )
    }

    override fun handleApplicationClass(clazz: Clazz) {
        LOGGER.warn(
            "Duplicated application class definition found: [{}] at {}, previously defined at {}",
            clazz.className,
            clazz.filePath,
            getExistingClassPath(clazz)
        )
    }

    companion object {
        private val LOGGER = LoggerFactory.getLogger(DuplicateClassPrinter::class.java)!!
    }
}
