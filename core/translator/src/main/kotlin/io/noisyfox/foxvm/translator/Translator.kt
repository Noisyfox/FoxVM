package io.noisyfox.foxvm.translator

import io.noisyfox.foxvm.bytecode.ClassPool
import io.noisyfox.foxvm.bytecode.CombinedClassPool
import io.noisyfox.foxvm.bytecode.SimpleClassPool
import io.noisyfox.foxvm.bytecode.resolver.PreResolver
import io.noisyfox.foxvm.bytecode.visitor.ClassPoolFiller
import io.noisyfox.foxvm.bytecode.visitor.ClassPresenceFilter
import io.noisyfox.foxvm.translator.cgen.CMakeListsWriter
import io.noisyfox.foxvm.translator.cgen.ClassInfoWriter
import io.noisyfox.foxvm.translator.cgen.ClassWriter
import org.slf4j.LoggerFactory
import java.io.File
import java.io.IOException

class Translator(
    private val isRt: Boolean,
    private val runtimeClasspath: Array<File>,
    private val inClasspath: Array<File>,
    private val outputPath: File
) {

    private val runtimeClassPool: ClassPool = SimpleClassPool()
    private val applicationClassPool: ClassPool = SimpleClassPool()
    private val fullClassPool: ClassPool = CombinedClassPool(runtimeClassPool, applicationClassPool)

    fun execute() {
        loadClasses()
        preResolve()

        // Create output dir
        if (!outputPath.exists() && !outputPath.mkdirs()) {
            LOGGER.error("Unable to create output directory {}.", outputPath)
            throw IOException("Unable to create output directory ${outputPath}.")
        }

        writeClasses()

        // Write cmakelists
        applicationClassPool.accept(CMakeListsWriter(isRt = isRt, outputDir = outputPath))
    }

    /**
     * Read all classes
     */
    private fun loadClasses() {
        val classFiller = SupportedClassFilter(
            ClassPresenceFilter(
                fullClassPool,
                DuplicateClassPrinter(fullClassPool),
                ClassPoolFiller(fullClassPool)
            )
        )

        // Class from inClasspath has higher priority
        ClasspathVisitor(isRuntimeClass = false, classpath = inClasspath).accept(classFiller)
        ClasspathVisitor(isRuntimeClass = true, classpath = runtimeClasspath).accept(classFiller)
    }

    /**
     * Pre-resolve all application classes
     */
    private fun preResolve() {
        val preResolver = PreResolver(fullClassPool)
        applicationClassPool.accept(preResolver)
        LOGGER.info("Pre-resolved {} classes.", preResolver.resolvedClasses)
    }

    /**
     * Write all application classes to C file
     */
    private fun writeClasses() {
        // Write each classes
        applicationClassPool.accept(ClassWriter(isRt = isRt, classPool = fullClassPool, outputDir = outputPath))

        // Write header contains ref to all classes
        applicationClassPool.accept(ClassInfoWriter(isRt = isRt, outputDir = outputPath))
    }

    companion object {
        private val LOGGER = LoggerFactory.getLogger(Translator::class.java)!!
    }
}
