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
import io.noisyfox.foxvm.translator.cgen.ConstantPoolWriter
import io.noisyfox.foxvm.translator.cgen.StringConstantPool
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
    private val constantPool: StringConstantPool = StringConstantPool()

    fun execute() {
        loadClasses()
        preResolve()

        // Create output dir
        if (!outputPath.exists() && !outputPath.mkdirs()) {
            LOGGER.error("Unable to create output directory {}.", outputPath)
            throw IOException("Unable to create output directory ${outputPath}.")
        }

        writeClasses()

        writeConstantPool()

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
        LOGGER.info("Pre-resolved {} class(es).", preResolver.resolvedClasses)
    }

    /**
     * Write all application classes to C file
     */
    private fun writeClasses() {
        // Write each classes
        ClassWriter(isRt = isRt, classPool = fullClassPool, constantPool = constantPool, outputDir = outputPath).also {
            applicationClassPool.accept(it)

            if (it.updatedClassNumber == 0) {
                LOGGER.info("{} class(es), {} method(s) and {} instruction(s) translated, all up-to-date.",
                    it.processedClassNumber, it.processedMethodNumber, it.processedInstructionNumber)
            } else {
                LOGGER.info(
                    "{} class(es), {} method(s) and {} instruction(s) translated, {} class(es) updated.",
                    it.processedClassNumber, it.processedMethodNumber, it.processedInstructionNumber,
                    it.updatedClassNumber
                )
            }
        }

        // Write files contain ref to all classes
        applicationClassPool.accept(ClassInfoWriter(isRt = isRt, outputDir = outputPath))
    }

    private fun writeConstantPool() {
        ConstantPoolWriter(isRt = isRt, constantPool = constantPool, outputDir = outputPath).write()
    }

    companion object {
        private val LOGGER = LoggerFactory.getLogger(Translator::class.java)!!
    }
}
