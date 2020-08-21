package io.noisyfox.foxvm.translator.cgen

import io.noisyfox.foxvm.bytecode.clazz.Clazz
import io.noisyfox.foxvm.bytecode.visitor.ClassPoolVisitor
import java.io.File

class CMakeListsWriter(
    outputDir: File
): ClassPoolVisitor {
    private val fileWriter = File(outputDir, "CMakeLists.txt").bufferedWriter()
    override fun visitStart() {
        fileWriter.write(
            """
                    |cmake_minimum_required(VERSION 3.1.0)
                    |project(foxvm_app C)
                    |set(CMAKE_C_STANDARD 99)
                    |
                    |include_directories(${'$'}<TARGET_PROPERTY:foxvm_runtime,INCLUDE_DIRECTORIES>)
                    |
                    |set(SOURCE_FILES_APP
                    |""".trimMargin()
        )
    }

    override fun handleApplicationClass(clazz: Clazz) {
        val info = clazz.requireClassInfo()
        fileWriter.write(
            """
                    |    _${info.cIdentifier}.c
                    |""".trimMargin()
        )
    }

    override fun visitEnd() {
        fileWriter.write(
            """
                    |    )
                    |
                    |add_c_library(
                    |    foxvm_app
                    |    OUTPUT_NAME app
                    |    PKGCONFIG_NAME foxvm_app
                    |    VERSION 0.0.1
                    |    
                    |    SOURCES
                    |    ${"$"}{SOURCE_FILES_APP}
                    |)
                    |""".trimMargin()
        )
        fileWriter.close()
    }
}