package io.noisyfox.foxvm.translator.cgen

import io.noisyfox.foxvm.bytecode.ClassPool
import io.noisyfox.foxvm.bytecode.asCString
import io.noisyfox.foxvm.bytecode.clazz.ClassInfo
import io.noisyfox.foxvm.bytecode.clazz.Clazz
import io.noisyfox.foxvm.bytecode.clazz.MethodInfo
import io.noisyfox.foxvm.bytecode.visitor.ClassHandler
import org.slf4j.LoggerFactory
import java.io.File
import java.io.Writer

/**
 * Must be synced with .\native\runtime\include\vm_base.h
 */
class ClassWriter(
    private val classPool: ClassPool,
    private val outputDir: File
) : ClassHandler {

    override fun handleApplicationClass(clazz: Clazz) {
        val info = clazz.requireClassInfo()

        // Generate header file
        File(outputDir, "_${info.cIdentifier}.h").bufferedWriter().use { headerWriter ->
            writeH(headerWriter, clazz, info)
        }

        // Generate C file
        File(outputDir, "_${info.cIdentifier}.c").bufferedWriter().use { cWriter ->
            writeC(cWriter, clazz, info)
        }
    }

    private fun writeH(headerWriter: Writer, clazz: Clazz, info: ClassInfo) {
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

        // Write includes
        headerWriter.write(
            """
                    |#include "vm_base.h"
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
                    |    ${field.cNameEnum(info)} = ${i},    // Static field ${info.fields[field.fieldIndex].name}: ${info.fields[field.fieldIndex].descriptor}
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
                    |    ${field.cNameEnum} = ${i},    // Instance field ${info.fields[field.fieldIndex].name}: ${info.fields[field.fieldIndex].descriptor}
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

        // Generate class definition
        headerWriter.write(
            """
                    |// Class definition
                    |typedef struct _${info.cClassName} {
                    |    JAVA_CLASS clazz;
                    |    void *monitor;
                    |
                    |    JavaClassInfo *info;
                    |
                    |    JAVA_OBJECT classLoader;
                    |
                    |    JAVA_CLASS superClass;
                    |    int interfaceCount;
                    |    JAVA_CLASS *interfaces;
                    |
                    |    uint16_t staticFieldCount;
                    |    ResolvedStaticField *staticFields;
                    |    JAVA_BOOLEAN hasStaticReference;
                    |
                    |    uint32_t fieldCount;
                    |    ResolvedInstanceField *fields;
                    |    JAVA_BOOLEAN hasReference;
                    |
                    |    uint32_t staticFieldRefCount;
                    |    ResolvedStaticFieldReference *staticFieldReferences;
                    |
                    |    // Start of resolved data storage
                    |""".trimMargin()
        )

        // Write field of storing the resolved data
        if (info.interfaces.isNotEmpty()) {
            headerWriter.write(
                """
                    |    JAVA_CLASS $CNAME_BACKING_INTERFACES[${info.interfaces.size}];
                    |""".trimMargin()
            )
        }
        if (info.preResolvedStaticFields.isNotEmpty()) {
            headerWriter.write(
                """
                    |    ResolvedStaticField $CNAME_BACKING_STATIC_FIELDS[${info.preResolvedStaticFields.size}];
                    |""".trimMargin()
            )
        }
        if (info.preResolvedInstanceFields.isNotEmpty()) {
            headerWriter.write(
                """
                    |    ResolvedInstanceField $CNAME_BACKING_INSTANCE_FIELDS[${info.preResolvedInstanceFields.size}];
                    |""".trimMargin()
            )
        }
        // TODO: Write resolved static field reference

        // Write static field data storage
        headerWriter.write(
            """
                    |
                    |    // Start of static field storage
                    |""".trimMargin()
        )
        if (info.preResolvedStaticFields.isNotEmpty()) {
            info.preResolvedStaticFields.forEach {
                headerWriter.write(
                    """
                    |    ${it.cStorageType(info)} ${it.cName(info)};
                    |""".trimMargin()
                )
            }
        }

        headerWriter.write(
            """
                    |} ${info.cClassName};
                    |
                    |""".trimMargin()
        )

        // Generate object definition
        headerWriter.write(
            """
                    |// Object definition
                    |typedef struct _${info.cObjectName} {
                    |    JAVA_CLASS clazz;
                    |    void *monitor;
                    |
                    |    // Start of instance field storage
                    |""".trimMargin()
        )

        if (info.preResolvedInstanceFields.isNotEmpty()) {
            info.preResolvedInstanceFields.forEach {
                headerWriter.write(
                    """
                    |    ${it.cStorageType} ${it.cName};
                    |""".trimMargin()
                )
            }
        }

        headerWriter.write(
            """
                    |} ${info.cObjectName};
                    |
                    |""".trimMargin()
        )

        // Generate method declaration
        if (info.methods.isNotEmpty()) {
            headerWriter.write(
                """
                    |// Method declarations
                    |""".trimMargin()
            )

            info.methods.forEach {
                headerWriter.write(
                    """
                    |${it.cDeclaration(info)};    // ${clazz.className}.${it.name}:${it.descriptor}
                    |""".trimMargin()
                )
            }

            headerWriter.write(
                """
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

    private fun writeC(cWriter: Writer, clazz: Clazz, info: ClassInfo) {
        cWriter.write(
            """
                    |// Generated from class [${clazz.className}] at ${clazz.filePath}
                    |
                    |""".trimMargin()
        )

        // Write includes
        cWriter.write(
            """
                    |#include "_${info.cIdentifier}.h"
                    |
                    |""".trimMargin()
        )

        // Write declarations
        cWriter.write(
            """
                    |// Declaration of the class resolve handler
                    |static JAVA_VOID ${info.cNameResolveHandler}(JAVA_CLASS c);
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
                    |    ${it.classInfo!!.cName},
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
                    |    {
                    |        .accessFlags = ${AccFlag.translateFieldAcc(it.access)},
                    |        .name = "${it.name.asCString()}",
                    |        .descriptor = "${it.descriptor.toString().asCString()}",
                    |    },
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

        // Write method info
        if (info.methods.isNotEmpty()) {
            cWriter.write(
                """
                    |// Methods of this class
                    |static MethodInfo ${info.cNameMethods}[] = {
                    |""".trimMargin()
            )

            info.methods.forEach {
                cWriter.write(
                    """
                    |    {
                    |        .accessFlags = ${AccFlag.translateMethodAcc(it.access)},
                    |        .name = "${it.name.asCString()}",
                    |        .descriptor = "${it.descriptor.toString().asCString()}",
                    |        .code = ${it.cName(info)},
                    |    },
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
                    |    {
                    |        .fieldIndex = ${it.fieldIndex},
                    |        .isReference = ${it.isReference.translate()},
                    |    },
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
                    |    {
                    |        .declaringClass = $dc,
                    |        .fieldIndex = ${it.fieldIndex},
                    |        .isReference = ${it.isReference.translate()},
                    |    },
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
                    |    .accessFlags = ${AccFlag.translateClassAcc(clazz.access)},
                    |    .thisClass = "${clazz.className.asCString()}",
                    |    .superClass = ${info.cSuperClass},
                    |    .interfaceCount = ${info.interfaces.size},
                    |    .interfaces = ${info.cNameInterfaces},
                    |
                    |    .fieldCount = ${info.fields.size},
                    |    .fields = ${info.cNameFields},
                    |
                    |    .methodCount =  ${info.methods.size},
                    |    .methods = ${info.cNameMethods},
                    |
                    |    .resolveHandler = ${info.cNameResolveHandler},
                    |    .classSize = sizeof(${info.cClassName}),
                    |    .instanceSize = sizeof(${info.cObjectName}),
                    |
                    |    .preResolvedStaticFieldCount = ${info.preResolvedStaticFields.size},
                    |    .preResolvedStaticFields = ${info.cNameStaticFields},
                    |
                    |    .preResolvedInstanceFieldCount = ${info.preResolvedInstanceFields.size},
                    |    .preResolvedInstanceFields = ${info.cNameInstanceFields},
                    |
                    |    .preResolvedStaticFieldRefCount = 0,
                    |    .preResolvedStaticFieldReferences = ${CNull},
                    |
                    |    .finalizer = ${info.cNameFinalizer},
                    |};
                    |
                    |""".trimMargin()
        )

        // Write definition of class resolve handler
        cWriter.write(
            """
                    |static JAVA_VOID ${info.cNameResolveHandler}(JAVA_CLASS c) {
                    |    ${info.cClassName} *clazz = (${info.cClassName} *)c;
                    |
                    |""".trimMargin()
        )

        // Setup interfaces
        if (info.interfaces.isNotEmpty()) {
            cWriter.write(
                """
                    |    clazz->interfaceCount = ${info.interfaces.size};
                    |    clazz->interfaces = clazz->$CNAME_BACKING_INTERFACES;
                    |
                    |""".trimMargin()
            )
        } else {
            cWriter.write(
                """
                    |    clazz->interfaceCount = 0;
                    |    clazz->interfaces = $CNull;
                    |
                    |""".trimMargin()
            )
        }

        // Resolve static fields
        if (info.preResolvedStaticFields.isNotEmpty()) {
            cWriter.write(
                """
                    |    clazz->staticFieldCount = ${info.preResolvedStaticFields.size};
                    |    clazz->staticFields = clazz->$CNAME_BACKING_STATIC_FIELDS;
                    |""".trimMargin()
            )
            // Resolve offset
            var hasRef = false
            info.preResolvedStaticFields.forEachIndexed { i, field ->
                cWriter.write(
                    """
                    |    clazz->staticFields[$i].info = &${info.cNameStaticFields}[$i];
                    |    clazz->staticFields[$i].offset = offsetof(${info.cClassName}, ${field.cName(info)});
                    |""".trimMargin()
                )
                hasRef = hasRef || field.isReference
            }
            cWriter.write(
                """
                    |    clazz->hasStaticReference = ${hasRef.translate()};
                    |
                    |""".trimMargin()
            )
        } else {
            cWriter.write(
                """
                    |    clazz->staticFieldCount = 0;
                    |    clazz->staticFields = $CNull;
                    |    clazz->hasStaticReference = ${false.translate()};
                    |
                    |""".trimMargin()
            )
        }

        // Resolve instance fields
        if (info.preResolvedInstanceFields.isNotEmpty()) {
            cWriter.write(
                """
                    |    clazz->fieldCount = ${info.preResolvedInstanceFields.size};
                    |    clazz->fields = clazz->$CNAME_BACKING_INSTANCE_FIELDS;
                    |""".trimMargin()
            )
            // Resolve offset
            var hasRef = false
            info.preResolvedInstanceFields.forEachIndexed { i, field ->
                cWriter.write(
                    """
                    |    clazz->fields[$i].info = &${info.cNameInstanceFields}[$i];
                    |    clazz->fields[$i].offset = offsetof(${info.cObjectName}, ${field.cName});
                    |""".trimMargin()
                )
                hasRef = hasRef || field.isReference
            }
            cWriter.write(
                """
                    |    clazz->hasReference = ${hasRef.translate()};
                    |
                    |""".trimMargin()
            )
        } else {
            cWriter.write(
                """
                    |    clazz->fieldCount = 0;
                    |    clazz->fields = $CNull;
                    |    clazz->hasReference = ${false.translate()};
                    |
                    |""".trimMargin()
            )
        }

        // TODO: resolve staticFieldReferences

        // TODO: set static const field to its initial value

        cWriter.write(
            """
                    |}
                    |
                    |""".trimMargin()
        )

        // Write implementation of non-native methods
        if (info.methods.isNotEmpty()) {
            cWriter.write(
                """
                    |// Method implementations
                    |
                    |""".trimMargin()
            )

            info.methods.forEach {
                writeMethodImpl(cWriter, info, it)
            }
        }
    }

    private fun writeMethodImpl(cWriter: Writer, clazzInfo: ClassInfo, method: MethodInfo) {
        val clazz = clazzInfo.thisClass

        cWriter.write(
            """
                    |// ${clazz.className}.${method.name}:${method.descriptor}
                    |${method.cDeclaration(clazzInfo)} {
                    |""".trimMargin()
        )

        // TODO: write instructions
        cWriter.write(
            """
                    |    return 0;
                    |""".trimMargin()
        )

        cWriter.write(
            """
                    |}
                    |
                    |""".trimMargin()
        )
    }

    companion object {
        private val LOGGER = LoggerFactory.getLogger(ClassWriter::class.java)!!

        private const val CNAME_BACKING_INTERFACES = "backingInterfaces"
        private const val CNAME_BACKING_STATIC_FIELDS = "backingStaticFields"
        private const val CNAME_BACKING_INSTANCE_FIELDS = "backingFields"
    }
}
