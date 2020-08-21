package io.noisyfox.foxvm.translator.cgen

import io.noisyfox.foxvm.bytecode.clazz.Clazz
import io.noisyfox.foxvm.bytecode.visitor.ClassHandler
import io.noisyfox.foxvm.bytecode.visitor.ClassPoolVisitor
import java.io.Closeable
import java.io.File

/**
 * Generate header file that contains decl of all application class infos.
 */
class ClassInfoHeaderWriter(
    outputDir: File
): ClassPoolVisitor {
    private val fileWriter = File(outputDir, FILE_NAME).bufferedWriter()

    override fun visitStart() {
        fileWriter.write(
            """
                    |#ifndef _FVM_APP_CLASS_INFO_H
                    |#define _FVM_APP_CLASS_INFO_H
                    |
                    |#include "vm_base.h"
                    |
                    |""".trimMargin()
        )
    }

    override fun handleApplicationClass(clazz: Clazz) {
        val info = clazz.requireClassInfo()
        fileWriter.write(
            """
                    |extern JavaClassInfo ${info.cName};
                    |""".trimMargin()
        )
    }

    override fun visitEnd() {
        fileWriter.write(
            """
                    |
                    |#endif //_FVM_APP_CLASS_INFO_H
                    |""".trimMargin()
        )
        fileWriter.close()
    }

    companion object {
        const val FILE_NAME = "fvm_app_class_info.h"
    }
}