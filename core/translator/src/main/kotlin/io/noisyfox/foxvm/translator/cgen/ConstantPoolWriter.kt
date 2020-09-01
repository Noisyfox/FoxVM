package io.noisyfox.foxvm.translator.cgen

import io.noisyfox.foxvm.translator.DiffFileWriter
import java.io.File

class ConstantPoolWriter(
    private val isRt: Boolean,
    private val constantPool: StringConstantPool,
    private val outputDir: File
) {
    fun write() {
        // Generate header file
        DiffFileWriter(File(outputDir, headerNameOf(isRt))).buffered().use { headerWriter ->
            headerWriter.write(
                """
                    |#ifndef ${headerGuardOf(isRt)}
                    |#define ${headerGuardOf(isRt)}
                    |
                    |#include "vm_base.h"
                    |
                    |extern JAVA_INT ${constantPoolPrefixOf(isRt)}_count;
                    |extern C_CSTR ${constantPoolPrefixOf(isRt)}[];
                    |extern JAVA_LONG ${constantPoolPrefixOf(isRt)}_init_thread[];
                    |extern JAVA_OBJECT ${constantPoolPrefixOf(isRt)}_obj[];
                    |
                    |#endif //${headerGuardOf(isRt)}
                    |
                    |""".trimMargin()
            )
        }

        // Generate C file
        DiffFileWriter(File(outputDir, cNameOf(isRt))).buffered().use { cWriter ->
            cWriter.write(
                """
                    |#include "${headerNameOf(isRt)}"
                    |
                    |JAVA_INT ${constantPoolPrefixOf(isRt)}_count = ${constantPool.count};
                    |
                    |C_CSTR ${constantPoolPrefixOf(isRt)}[] = {
                    |""".trimMargin()
            )
            constantPool.forEachIndexed { index, s ->
                cWriter.write(
                    """
                    |    /* $index */ ${s.toCString()},
                    |""".trimMargin()
                )
            }
            cWriter.write(
                """
                    |};
                    |
                    |JAVA_LONG ${constantPoolPrefixOf(isRt)}_init_thread[${constantPool.count}] = {0};
                    |JAVA_OBJECT ${constantPoolPrefixOf(isRt)}_obj[${constantPool.count}] = {0};
                    |
                    |""".trimMargin()
            )
        }
    }

    companion object {

        fun headerNameOf(isRt: Boolean): String = if (isRt) {
            "fvm_rt_constant_pool.h"
        } else {
            "fvm_app_constant_pool.h"
        }

        fun cNameOf(isRt: Boolean): String = if (isRt) {
            "fvm_rt_constant_pool.c"
        } else {
            "fvm_app_constant_pool.c"
        }

        fun constantPoolPrefixOf(isRt: Boolean): String = if (isRt) {
            "foxvm_constant_pool_rt"
        } else {
            "foxvm_constant_pool_app"
        }

        private fun headerGuardOf(isRt: Boolean): String = if (isRt) {
            "_FVM_RT_CONSTANT_POOL_H"
        } else {
            "_FVM_APP_CONSTANT_POOL_H"
        }
    }
}
