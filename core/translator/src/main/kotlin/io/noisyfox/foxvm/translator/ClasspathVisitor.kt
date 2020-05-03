package io.noisyfox.foxvm.translator

import io.noisyfox.foxvm.bytecode.clazz.Clazz
import io.noisyfox.foxvm.bytecode.visitor.ClassHandler
import org.apache.tools.ant.DirectoryScanner
import org.slf4j.LoggerFactory
import java.io.File
import java.io.IOException
import java.io.InputStream
import java.util.zip.ZipInputStream

class ClasspathVisitor(
    private val isRuntimeClass: Boolean,
    private val classpath: Array<File>
) {
    @Throws(IOException::class)
    fun accept(handler: ClassHandler) {
        classpath.forEach {
            visitFile(it, handler)
        }
    }

    @Throws(IOException::class)
    private fun visitFile(cp: File, handler: ClassHandler) {
        if (cp.isDirectory) {
            visitFolder(cp, handler)
        } else if (cp.extension.toLowerCase() == "jar" || cp.extension.toLowerCase() == "zip") {
            visitZip(cp, handler)
        } else {
            throw IOException("Unsupported file $cp")
        }
    }

    /**
     * Visit all .class files under the given directory
     */
    @Throws(IOException::class)
    private fun visitFolder(folder: File, handler: ClassHandler) {
        val ds = DirectoryScanner()
        ds.setIncludes(arrayOf("**\\*.class"))
        ds.basedir = folder
        ds.scan()

        ds.includedFiles.forEach {
            val f = File(folder, it)
            LOGGER.debug("Read class file {}", f)

            f.inputStream().use { i ->
                visitClass(
                    filePath = f.absolutePath,
                    inputStream = i.buffered(),
                    handler = handler
                )
            }
        }
    }

    /**
     * Visit all .class files inside the given zip file
     */
    @Throws(IOException::class)
    private fun visitZip(zipFile: File, handler: ClassHandler) {
        zipFile.inputStream().use {
            val zipIn = ZipInputStream(it.buffered())

            while (true) {
                val entry = zipIn.nextEntry ?: break

                if (entry.isDirectory || !entry.name.toLowerCase().endsWith(".class")) {
                    continue
                }
                LOGGER.debug("Read class file {}!{}", zipFile, entry.name)

                visitClass(
                    filePath = "$zipFile!${entry.name}",
                    inputStream = zipIn,
                    handler = handler
                )
            }
        }
    }

    private fun visitClass(
        filePath: String,
        inputStream: InputStream,
        handler: ClassHandler
    ) {
        Clazz(
            isRuntimeClass = isRuntimeClass,
            filePath = filePath,
            inputStream = inputStream
        ).accept(handler)
    }

    companion object {
        private val LOGGER = LoggerFactory.getLogger(ClasspathVisitor::class.java)!!
    }
}
