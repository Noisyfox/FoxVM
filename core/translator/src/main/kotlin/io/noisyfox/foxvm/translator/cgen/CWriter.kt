package io.noisyfox.foxvm.translator.cgen

import io.noisyfox.foxvm.bytecode.clazz.ClassInfo
import java.io.Writer

class CWriter(
    private val writer: Writer
) : AutoCloseable {

    private var isFinished = false
    private val includeHeaders: LinkedHashSet<String> = LinkedHashSet()
    private var contentAfterDependency: StringBuilder? = null

    fun addDependency(clazz: ClassInfo): CWriter {
        return addDependency("_${clazz.cIdentifier}.h")
    }

    fun addDependency(header: String): CWriter {
        require(!isFinished)

        includeHeaders.add(header)

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
        includeHeaders.forEach { header ->
            writer.write(
                """
                    |#include "$header"
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