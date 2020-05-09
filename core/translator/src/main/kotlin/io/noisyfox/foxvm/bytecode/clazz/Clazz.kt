package io.noisyfox.foxvm.bytecode.clazz

import io.noisyfox.foxvm.bytecode.visitor.ClassHandler
import org.objectweb.asm.ClassReader
import org.objectweb.asm.Opcodes
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

    override fun toString(): String {
        return "Class [${className}] (from: $filePath)"
    }

    companion object {
        const val CLASS_JAVA_LANG_OBJECT = "java/lang/Object"
        const val CLASS_JAVA_LANG_ENUM = "java/lang/Enum"
    }
}


