package io.noisyfox.foxvm.translator.cgen

import io.noisyfox.foxvm.bytecode.clazz.Clazz
import io.noisyfox.foxvm.bytecode.visitor.ClassPoolVisitor
import io.noisyfox.foxvm.translator.DiffFileWriter
import java.io.File

class CMakeListsWriter(
    private val isRt: Boolean,
    outputDir: File
): ClassPoolVisitor {
    private val fileWriter = DiffFileWriter(File(outputDir, "CMakeLists.txt")).buffered()
    override fun visitStart() {
        fileWriter.write(
            """
                    |cmake_minimum_required(VERSION 3.1.0)
                    |project(${libraryNameOf(isRt)} C)
                    |set(CMAKE_C_STANDARD 99)
                    |
                    |include_directories(${'$'}<TARGET_PROPERTY:foxvm_runtime,INCLUDE_DIRECTORIES>)
                    |
                    |set(${sourceNameOf(isRt)}
                    |    ${ClassInfoWriter.cNameOf(isRt)}
                    |    ${ConstantPoolWriter.cNameOf(isRt)}
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
                    |    ${libraryNameOf(isRt)}
                    |    OUTPUT_NAME ${outputNameOf(isRt)}
                    |    PKGCONFIG_NAME ${libraryNameOf(isRt)}
                    |    VERSION 0.0.1
                    |    
                    |    SOURCES
                    |    ${"$"}{${sourceNameOf(isRt)}}
                    |
                    |    LOCAL_LIBRARIES
                    |    foxvm_runtime
                    |)
                    |""".trimMargin()
        )
        fileWriter.close()
    }

    companion object {
        private fun sourceNameOf(isRt: Boolean): String = if(isRt){
            "SOURCE_FILES_RT"
        } else {
            "SOURCE_FILES_APP"
        }

        private fun libraryNameOf(isRt: Boolean): String = if(isRt){
            "foxvm_rt"
        } else {
            "foxvm_app"
        }

        private fun outputNameOf(isRt: Boolean): String = if(isRt){
            "rt"
        } else {
            "app"
        }
    }
}