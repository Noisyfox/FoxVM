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
}


