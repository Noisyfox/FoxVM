package io.noisyfox.foxvm.translator.cgen

import io.noisyfox.foxvm.bytecode.clazz.ClassInfo
import java.io.Writer

class CWriter(
    private val writer: Writer
) : AutoCloseable {

    private var isFinished = false
    private val dependencies: LinkedHashMap<String, ClassInfo> = LinkedHashMap()
    private var contentAfterDependency: StringBuilder? = null

    fun addDependency(clazz: ClassInfo): CWriter {
        require(!isFinished)

        val name = clazz.thisClass.className
        if (name !in dependencies) {
            dependencies[name] = clazz
        }

        return this
    }

    fun includeDependenciesHere(): CWriter {
        require(!isFinished)
        require(contentAfterDependency == null)
        contentAfterDependency = StringBuilder()

        return this
    }

    fun write(content: String): CWriter {
        require(!isFinished)

        contentAfterDependency?.also {
            it.append(content)
        } ?: run {
            writer.write(content)
        }

        return this
    }

    fun finish() {
        if (isFinished) {
            return
        }
        isFinished = true
        dependencies.forEach { (_, clazz) ->
            writer.write(
                """
                    |#include "_${clazz.cIdentifier}.h"
                    |""".trimMargin()
            )
        }

        contentAfterDependency?.also {
            writer.write(it.toString())
        }
    }

    override fun close() {
        finish()
        writer.close()
    }
}