package io.noisyfox.foxvm.translator.cgen

import io.noisyfox.foxvm.bytecode.ClassPool
import io.noisyfox.foxvm.bytecode.clazz.Clazz
import io.noisyfox.foxvm.bytecode.visitor.ClassHandler
import org.slf4j.LoggerFactory
import java.io.File

/**
 * Must be synced with .\native\runtime\include\vm_base.h
 */
class ClassWriter(
    private val classPool: ClassPool,
    private val outputDir: File
) : ClassHandler {
    override fun handleApplicationClass(clazz: Clazz) {
        val info = requireNotNull(clazz.classInfo)

        // Generate C file
        File(outputDir, "_${info.cIdentifier}.c").bufferedWriter().use { cWriter ->
            cWriter.write(
                """
                    |// Generated from class [${clazz.className}] at ${clazz.filePath}
                    |
                    |""".trimMargin()
            )

            // Write includes
            cWriter.write(
                """
                    |#include "vm_base.h"
                    |
                    |""".trimMargin()
            )

            // Write interfaces
            if (info.interfaces.isNotEmpty()) {
                cWriter.write(
                    """
                    |// Interfaces of this class
                    |static JavaClassInfo* ${info.cNameInterfaces}[] = {
                    |""".trimMargin()
                )

                info.interfaces.forEach {
                    cWriter.write(
                        """
                    |   ${it.classInfo!!.cName},
                    |""".trimMargin()
                    )
                }

                cWriter.write(
                    """
                    |};
                    |
                    |""".trimMargin()
                )
            }

            // Write field info
            if (info.fields.isNotEmpty()) {
                cWriter.write(
                    """
                    |// Fields of this class
                    |static FieldInfo ${info.cNameFields}[] = {
                    |""".trimMargin()
                )

                info.fields.forEach {
                    cWriter.write(
                        """
                    |   {
                    |       .accessFlags = ${AccFlag.translateFieldAcc(it.access)},
                    |       .name = "${it.name}",
                    |       .descriptor = "${it.descriptor}",
                    |   },
                    |""".trimMargin()
                    )
                }

                cWriter.write(
                    """
                    |};
                    |
                    |""".trimMargin()
                )
            }

            // TODO: Write method info

            // Write pre-resolved static field info
            if (info.preResolvedStaticFields.isNotEmpty()) {
                cWriter.write(
                    """
                    |// Pre-resolved static fields of this class
                    |static PreResolvedStaticFieldInfo ${info.cNameStaticFields}[] = {
                    |""".trimMargin()
                )

                info.preResolvedStaticFields.forEach {
                    cWriter.write(
                        """
                    |   {
                    |       .fieldIndex = ${it.fieldIndex},
                    |       .isReference = ${it.isReference.translate()},
                    |   },
                    |""".trimMargin()
                    )
                }

                cWriter.write(
                    """
                    |};
                    |
                    |""".trimMargin()
                )
            }

            // Write pre-resolved instance field info
            if (info.preResolvedInstanceFields.isNotEmpty()) {
                cWriter.write(
                    """
                    |// Pre-resolved instance fields of this class
                    |static PreResolvedInstanceFieldInfo ${info.cNameInstanceFields}[] = {
                    |""".trimMargin()
                )

                info.preResolvedInstanceFields.forEach {
                    val dc = if (it.declaringClass == clazz) {
                        CNull
                    } else {
                        it.declaringClass.classInfo!!.cName
                    }
                    cWriter.write(
                        """
                    |   {
                    |       .declaringClass = $dc,
                    |       .fieldIndex = ${it.fieldIndex},
                    |       .isReference = ${it.isReference.translate()},
                    |   },
                    |""".trimMargin()
                    )
                }

                cWriter.write(
                    """
                    |};
                    |
                    |""".trimMargin()
                )
            }

            // TODO: Write pre-resolved static field reference

            // Write class info
            cWriter.write(
                """
                    |// Class info
                    |JavaClassInfo ${info.cName} = {
                    |   .accessFlags = ${AccFlag.translateClassAcc(clazz.access)},
                    |   .thisClass = "${clazz.className}",
                    |   .superClass = ${info.cSuperClass},
                    |   .interfaceCount = ${info.interfaces.size},
                    |   .interfaces = ${info.cNameInterfaces},
                    |   .fieldCount = ${info.fields.size},
                    |   .fields = ${info.cNameFields},
                    |   .methodCount = 0,
                    |   .methods = ${CNull},
                    |   .preResolvedStaticFieldCount = ${info.preResolvedStaticFields.size},
                    |   .preResolvedStaticFields = ${info.cNameStaticFields},
                    |   .preResolvedInstanceFieldCount = ${info.preResolvedInstanceFields.size},
                    |   .preResolvedInstanceFields = ${info.cNameInstanceFields},
                    |   .preResolvedStaticFieldRefCount = 0,
                    |   .preResolvedStaticFieldReferences = ${CNull},
                    |};
                    |
                    |""".trimMargin()
            )
        }

        // Generate header file
        File(outputDir, "_${info.cIdentifier}.h").bufferedWriter().use { headerWriter ->
            val includeGuard = "_${info.cIdentifier.toUpperCase()}_H"
            headerWriter.write(
                """
                    |// Generated from class [${clazz.className}] at ${clazz.filePath}
                    |
                    |#ifndef $includeGuard
                    |#define $includeGuard
                    |
                    |""".trimMargin()
            )

            // Generate static field index enum
            if (info.preResolvedStaticFields.isNotEmpty()) {
                headerWriter.write(
                    """
                    |// Index of static fields
                    |enum {
                    |""".trimMargin()
                )

                info.preResolvedStaticFields.forEachIndexed { i, field ->
                    headerWriter.write(
                        """
                    |   ${field.cNameEnum(info)} = ${i},
                    |""".trimMargin()
                    )
                }

                headerWriter.write(
                    """
                    |};
                    |
                    |""".trimMargin()
                )
            }

            // Generate instance field index enum, we only care about the fields declared by this class
            if (info.preResolvedInstanceFields.any { it.declaringClass == clazz }) {
                headerWriter.write(
                    """
                    |// Index of instance fields
                    |enum {
                    |""".trimMargin()
                )

                info.preResolvedInstanceFields.forEachIndexed { i, field ->
                    if (field.declaringClass == clazz) {
                        headerWriter.write(
                            """
                    |   ${field.cNameEnum()} = ${i},
                    |""".trimMargin()
                        )
                    }
                }

                headerWriter.write(
                    """
                    |};
                    |
                    |""".trimMargin()
                )
            }

            headerWriter.write(
                """
                    |#endif //$includeGuard
                    |""".trimMargin()
            )
        }
    }

    companion object {
        private val LOGGER = LoggerFactory.getLogger(ClassWriter::class.java)!!
    }
}
