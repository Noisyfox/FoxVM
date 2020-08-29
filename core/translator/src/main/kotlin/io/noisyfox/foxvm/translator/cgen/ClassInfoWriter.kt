package io.noisyfox.foxvm.translator.cgen

import io.noisyfox.foxvm.bytecode.clazz.Clazz
import io.noisyfox.foxvm.bytecode.visitor.ClassPoolVisitor
import io.noisyfox.foxvm.translator.DiffFileWriter
import java.io.File

/**
 * Generate header and C file that contains decl of all application class infos.
 */
class ClassInfoWriter(
    private val isRt: Boolean,
    outputDir: File
) : ClassPoolVisitor {
    private val headerWriter = DiffFileWriter(File(outputDir, headerNameOf(isRt))).buffered()
    private val cWriter = DiffFileWriter(File(outputDir, cNameOf(isRt))).buffered()

    override fun visitStart() {
        headerWriter.write(
            """
                    |#ifndef ${headerGuardOf(isRt)}
                    |#define ${headerGuardOf(isRt)}
                    |
                    |#include "vm_base.h"
                    |
                    |extern JavaClassInfo* ${classInfosNameOf(isRt)}[];
                    |
                    |""".trimMargin()
        )

        cWriter.write(
            """
                    |#include "${headerNameOf(isRt)}"
                    |
                    |JavaClassInfo* ${classInfosNameOf(isRt)}[] = {
                    |""".trimMargin()
        )
    }

    override fun handleApplicationClass(clazz: Clazz) {
        val info = clazz.requireClassInfo()
        headerWriter.write(
            """
                    |extern JavaClassInfo ${info.cName};
                    |""".trimMargin()
        )

        cWriter.write(
            """
                    |    &${info.cName},
                    |""".trimMargin()
        )
    }

    override fun visitEnd() {
        headerWriter.write(
            """
                    |
                    |#endif //${headerGuardOf(isRt)}
                    |""".trimMargin()
        )
        headerWriter.close()

        cWriter.write(
            """
                    |    NULL
                    |};
                    |""".trimMargin()
        )
        cWriter.close()
    }

    companion object {
        const val HEADER_NAME_RT = "fvm_rt_class_info.h"
        const val HEADER_NAME_APP = "fvm_app_class_info.h"
        const val C_NAME_RT = "fvm_rt_class_info.c"
        const val C_NAME_APP = "fvm_app_class_info.c"
        const val CLASS_INFOS_NAME_RT = "foxvm_class_infos_rt"
        const val CLASS_INFOS_NAME_APP = "foxvm_class_infos_app"

        fun headerNameOf(isRt: Boolean): String = if (isRt) {
            HEADER_NAME_RT
        } else {
            HEADER_NAME_APP
        }

        fun cNameOf(isRt: Boolean): String = if (isRt) {
            C_NAME_RT
        } else {
            C_NAME_APP
        }

        fun classInfosNameOf(isRt: Boolean): String = if (isRt) {
            CLASS_INFOS_NAME_RT
        } else {
            CLASS_INFOS_NAME_APP
        }

        private fun headerGuardOf(isRt: Boolean): String = if (isRt) {
            "_FVM_RT_CLASS_INFO_H"
        } else {
            "_FVM_APP_CLASS_INFO_H"
        }
    }
}