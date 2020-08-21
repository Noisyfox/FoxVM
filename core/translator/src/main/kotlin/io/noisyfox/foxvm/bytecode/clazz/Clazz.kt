package io.noisyfox.foxvm.bytecode.clazz

import io.noisyfox.foxvm.bytecode.visitor.ClassHandler
import org.objectweb.asm.ClassReader
import java.io.InputStream

class Clazz(
    val isRuntimeClass: Boolean,
    val filePath: String,
    inputStream: InputStream
) : ClassReader(inputStream) {

    var classInfo: ClassInfo? = null

    fun requireClassInfo(): ClassInfo = requireNotNull(classInfo) {
        "Class [${className}] not resolved yet!"
    }

    fun accept(classHandler: ClassHandler) {
        if (isRuntimeClass) {
            classHandler.handleRuntimeClass(this)
        } else {
            classHandler.handleApplicationClass(this)
        }
    }

    fun visitSuperClasses(classHandler: ClassHandler) {
        val info = requireClassInfo()
        val supers = LinkedHashSet<Clazz>()
        val superGatherer = object : ClassHandler {
            override fun handleAnyClass(clazz: Clazz) {
                supers.add(clazz)
            }
        }

        info.superClass?.let {
            it.visitSuperClasses(superGatherer)
            supers.add(it)
        }
        info.interfaces.forEach {
            it.visitSuperClasses(superGatherer)
            supers.add(it)
        }

        supers.forEach {
            it.accept(classHandler)
        }
    }

    override fun toString(): String {
        return "Class [${className}] (from: $filePath)"
    }

    companion object {
        const val CLASS_JAVA_LANG_OBJECT = "java/lang/Object"
        const val CLASS_JAVA_LANG_ENUM = "java/lang/Enum"
    }
}


