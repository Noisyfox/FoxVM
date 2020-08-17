package io.noisyfox.foxvm.translator.cgen

import io.noisyfox.foxvm.bytecode.ClassPool
import io.noisyfox.foxvm.bytecode.asCString
import io.noisyfox.foxvm.bytecode.clazz.ClassInfo
import io.noisyfox.foxvm.bytecode.clazz.Clazz
import io.noisyfox.foxvm.bytecode.clazz.MethodInfo
import io.noisyfox.foxvm.bytecode.visitor.ClassHandler
import org.objectweb.asm.Opcodes
import org.objectweb.asm.tree.IincInsnNode
import org.objectweb.asm.tree.InsnNode
import org.objectweb.asm.tree.IntInsnNode
import org.objectweb.asm.tree.JumpInsnNode
import org.objectweb.asm.tree.LabelNode
import org.objectweb.asm.tree.LineNumberNode
import org.objectweb.asm.tree.VarInsnNode
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
        val nonAbstractMethods = info.methods.filterNot { it.isAbstract }
        if (nonAbstractMethods.isNotEmpty()) {
            headerWriter.write(
                """
                    |// Method declarations
                    |""".trimMargin()
            )

            nonAbstractMethods.forEach {
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
                    |        .signature = ${it.signature.toCString()},
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
                val codeRef = if (it.isAbstract) {
                    CNull
                } else {
                    it.cName(info)
                }
                cWriter.write(
                    """
                    |    {
                    |        .accessFlags = ${AccFlag.translateMethodAcc(it.access)},
                    |        .name = "${it.name.asCString()}",
                    |        .descriptor = "${it.descriptor.toString().asCString()}",
                    |        .signature = ${it.signature.toCString()},
                    |        .code = $codeRef,
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
                    |    .signature = ${info.signature.toCString()},
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

        // Write implementation of concrete methods
        val concreteMethods = info.methods.withIndex().filter { it.value.isConcrete }
        if (concreteMethods.isNotEmpty()) {
            cWriter.write(
                """
                    |// Method implementations
                    |
                    |""".trimMargin()
            )

            concreteMethods.forEach {
                writeMethodImpl(cWriter, info, it.index, it.value)
            }
        }
    }

    private fun writeMethodImpl(cWriter: Writer, clazzInfo: ClassInfo, index: Int, method: MethodInfo) {
        val clazz = clazzInfo.thisClass
        val node = method.methodNode

        val stackStartStatement = if(node.maxStack == 0 && node.maxLocals == 0) {
            "stack_frame_start_zero(${index})"
        } else {
            "stack_frame_start(${index}, ${node.maxStack}, ${node.maxLocals})"
        }
        cWriter.write(
            """
                    |// ${clazz.className}.${method.name}:${method.descriptor}
                    |${method.cDeclaration(clazzInfo)} {
                    |   $stackStartStatement;
                    |""".trimMargin()
        )

        // Pass arguments
        val argumentTypes = method.descriptor.argumentTypes
        val argumentCount = if (method.isStatic) {
            argumentTypes.size
        } else {
            argumentTypes.size + 1 // implicitly passed this
        }
        if (argumentCount > 0) {
            cWriter.write(
                """
                    |   local_transfer_arguments(vmCurrentContext, ${argumentCount});
                    |""".trimMargin()
            )
        }

        // TODO: write instructions

        node.instructions.forEach { inst ->
            when (inst) {
                is LabelNode -> {
                    cWriter.write(
                        """
                    |
                    |${inst.cName(method)}:
                    |   bc_label(${inst.index(method)});
                    |""".trimMargin()
                    )
                }
                is LineNumberNode -> {
                    cWriter.write(
                        """
                    |   bc_line(${inst.line}); // Line ${inst.line}
                    |
                    |""".trimMargin()
                    )
                }
                is VarInsnNode -> {
                    when (inst.opcode) {
                        Opcodes.RET -> {
                            // TODO
                        }
                        in byteCodesVarInst -> {
                            val functionName = byteCodesVarInst[inst.opcode]
                            cWriter.write(
                                """
                    |   ${functionName}(${inst.`var`});
                    |""".trimMargin()
                            )
                        }
                        else -> throw IllegalArgumentException("Unexpected opcode ${inst.opcode}")
                    }
                }
                is IincInsnNode -> {
                    cWriter.write(
                        """
                    |   bc_iinc(${inst.`var`}, ${inst.incr});
                    |""".trimMargin()
                    )
                }
                is IntInsnNode -> {
                    when (inst.opcode) {
                        Opcodes.NEWARRAY -> {
                            // TODO
                        }
                        in byteCodesIntInst -> {
                            val functionName = byteCodesIntInst[inst.opcode]
                            cWriter.write(
                                """
                    |   ${functionName}(${inst.operand});
                    |""".trimMargin()
                            )
                        }
                        else -> throw IllegalArgumentException("Unexpected opcode ${inst.opcode}")
                    }
                }
                is InsnNode -> {
                    when (inst.opcode) {
                        in byteCodesInstInst -> {
                            val functionName = byteCodesInstInst[inst.opcode]
                            cWriter.write(
                                """
                    |   ${functionName}();
                    |""".trimMargin()
                            )
                        }
//                        else -> throw IllegalArgumentException("Unexpected opcode ${inst.opcode}")
                    }
                }
                is JumpInsnNode -> {
                    when (inst.opcode) {
                        in byteCodesJumpInst -> {
                            val functionName = byteCodesJumpInst[inst.opcode]
                            cWriter.write(
                                """
                    |   ${functionName}(${inst.label.cName(method)});
                    |""".trimMargin()
                            )
                        }
                        else -> throw IllegalArgumentException("Unexpected opcode ${inst.opcode}")
                    }
                }
            }
        }

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

        private val byteCodesVarInst = mapOf(
            Opcodes.ILOAD to "bc_iload",
            Opcodes.LLOAD to "bc_lload",
            Opcodes.FLOAD to "bc_fload",
            Opcodes.DLOAD to "bc_dload",
            Opcodes.ALOAD to "bc_aload",

            Opcodes.ISTORE to "bc_istore",
            Opcodes.LSTORE to "bc_lstore",
            Opcodes.FSTORE to "bc_fstore",
            Opcodes.DSTORE to "bc_dstore",
            Opcodes.ASTORE to "bc_astore"
        )

        private val byteCodesIntInst = mapOf(
            Opcodes.BIPUSH to "bc_bipush",
            Opcodes.SIPUSH to "bc_sipush"
        )

        private val byteCodesInstInst = mapOf(
            Opcodes.NOP to "bc_nop",

            Opcodes.ACONST_NULL to "bc_aconst_null",

            Opcodes.ICONST_M1 to "bc_iconst_m1",
            Opcodes.ICONST_0  to "bc_iconst_0",
            Opcodes.ICONST_1  to "bc_iconst_1",
            Opcodes.ICONST_2  to "bc_iconst_2",
            Opcodes.ICONST_3  to "bc_iconst_3",
            Opcodes.ICONST_4  to "bc_iconst_4",
            Opcodes.ICONST_5  to "bc_iconst_5",

            Opcodes.LCONST_0 to "bc_lconst_0",
            Opcodes.LCONST_1 to "bc_lconst_1",

            Opcodes.FCONST_0 to "bc_fconst_0",
            Opcodes.FCONST_1 to "bc_fconst_1",
            Opcodes.FCONST_2 to "bc_fconst_2",

            Opcodes.DCONST_0 to "bc_dconst_0",
            Opcodes.DCONST_1 to "bc_dconst_1",

            Opcodes.POP  to "bc_pop",
            Opcodes.POP2 to "bc_pop2",

            Opcodes.DUP     to "bc_dup",
            Opcodes.DUP_X1  to "bc_dup_x1",
            Opcodes.DUP_X2  to "bc_dup_x2",
            Opcodes.DUP2    to "bc_dup2",
            Opcodes.DUP2_X1 to "bc_dup2_x1",
            Opcodes.DUP2_X2 to "bc_dup2_x2",

            Opcodes.SWAP to "bc_swap",

            Opcodes.IADD to "bc_iadd",
            Opcodes.LADD to "bc_ladd",
            Opcodes.FADD to "bc_fadd",
            Opcodes.DADD to "bc_dadd",

            Opcodes.ISUB to "bc_isub",
            Opcodes.LSUB to "bc_lsub",
            Opcodes.FSUB to "bc_fsub",
            Opcodes.DSUB to "bc_dsub",

            Opcodes.IMUL to "bc_imul",
            Opcodes.LMUL to "bc_lmul",
            Opcodes.FMUL to "bc_fmul",
            Opcodes.DMUL to "bc_dmul",

            Opcodes.IDIV to "bc_idiv",
            Opcodes.LDIV to "bc_ldiv",
            Opcodes.FDIV to "bc_fdiv",
            Opcodes.DDIV to "bc_ddiv",

            Opcodes.IREM to "bc_irem",
            Opcodes.LREM to "bc_lrem",
            Opcodes.FREM to "bc_frem",
            Opcodes.DREM to "bc_drem",

            Opcodes.INEG to "bc_ineg",
            Opcodes.LNEG to "bc_lneg",
            Opcodes.FNEG to "bc_fneg",
            Opcodes.DNEG to "bc_dneg",

            Opcodes.ISHL to "bc_ishl",
            Opcodes.LSHL to "bc_lshl",

            Opcodes.ISHR to "bc_ishr",
            Opcodes.LSHR to "bc_lshr",

            Opcodes.IUSHR to "bc_iushr",
            Opcodes.LUSHR to "bc_lushr",

            Opcodes.IAND to "bc_iand",
            Opcodes.LAND to "bc_land",

            Opcodes.IOR to "bc_ior",
            Opcodes.LOR to "bc_lor",

            Opcodes.IXOR to "bc_ixor",
            Opcodes.LXOR to "bc_lxor",

            Opcodes.I2L to "bc_i2l",
            Opcodes.I2F to "bc_i2f",
            Opcodes.I2D to "bc_i2d",
            Opcodes.L2I to "bc_l2i",
            Opcodes.L2F to "bc_l2f",
            Opcodes.L2D to "bc_l2d",
            Opcodes.F2I to "bc_f2i",
            Opcodes.F2L to "bc_f2l",
            Opcodes.F2D to "bc_f2d",
            Opcodes.D2I to "bc_d2i",
            Opcodes.D2L to "bc_d2l",
            Opcodes.D2F to "bc_d2f",
            Opcodes.I2B to "bc_i2b",
            Opcodes.I2C to "bc_i2c",
            Opcodes.I2S to "bc_i2s",

            Opcodes.LCMP to "bc_lcmp",
            Opcodes.FCMPL to "bc_fcmpl",
            Opcodes.FCMPG to "bc_fcmpg",
            Opcodes.DCMPL to "bc_dcmpl",
            Opcodes.DCMPG to "bc_dcmpg"
        )

        private val byteCodesJumpInst = mapOf(
            Opcodes.IFEQ        to "bc_ifeq",
            Opcodes.IFNE        to "bc_ifne",
            Opcodes.IFLT        to "bc_iflt",
            Opcodes.IFLE        to "bc_ifle",
            Opcodes.IFGT        to "bc_ifgt",
            Opcodes.IFGE        to "bc_ifge",
            Opcodes.IF_ICMPEQ   to "bc_if_icmpeq",
            Opcodes.IF_ICMPNE   to "bc_if_icmpne",
            Opcodes.IF_ICMPLT   to "bc_if_icmplt",
            Opcodes.IF_ICMPLE   to "bc_if_icmple",
            Opcodes.IF_ICMPGT   to "bc_if_icmpgt",
            Opcodes.IF_ICMPGE   to "bc_if_icmpge",
            Opcodes.IF_ACMPEQ   to "bc_if_acmpeq",
            Opcodes.IF_ACMPNE   to "bc_if_acmpne",
            Opcodes.IFNULL      to "bc_ifnull",
            Opcodes.IFNONNULL   to "bc_ifnonnull",
            Opcodes.GOTO        to "bc_goto"
            // JSR not supported!
        )
    }
}
