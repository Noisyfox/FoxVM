package io.noisyfox.foxvm.translator.cgen

import io.noisyfox.foxvm.bytecode.ClassPool
import io.noisyfox.foxvm.bytecode.asCString
import io.noisyfox.foxvm.bytecode.clazz.ClassInfo
import io.noisyfox.foxvm.bytecode.clazz.Clazz
import io.noisyfox.foxvm.bytecode.clazz.MethodInfo
import io.noisyfox.foxvm.bytecode.visitor.ClassHandler
import org.objectweb.asm.Opcodes
import org.objectweb.asm.Type
import org.objectweb.asm.tree.FieldInsnNode
import org.objectweb.asm.tree.IincInsnNode
import org.objectweb.asm.tree.InsnNode
import org.objectweb.asm.tree.IntInsnNode
import org.objectweb.asm.tree.JumpInsnNode
import org.objectweb.asm.tree.LabelNode
import org.objectweb.asm.tree.LdcInsnNode
import org.objectweb.asm.tree.LineNumberNode
import org.objectweb.asm.tree.LookupSwitchInsnNode
import org.objectweb.asm.tree.MethodInsnNode
import org.objectweb.asm.tree.TableSwitchInsnNode
import org.objectweb.asm.tree.VarInsnNode
import org.slf4j.LoggerFactory
import java.io.File
import java.io.Writer

/**
 * Must be synced with .\native\runtime\include\vm_base.h
 */
class ClassWriter(
    private val isRt: Boolean,
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
                    |    ${field.cNameEnum} = ${i},    // Static field ${info.fields[field.fieldIndex].name}: ${info.fields[field.fieldIndex].descriptor}
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
        if (info.preResolvedInstanceFields.any { it.field.declaringClass == info }) {
            headerWriter.write(
                """
                    |// Index of instance fields
                    |enum {
                    |""".trimMargin()
            )

            info.preResolvedInstanceFields.forEachIndexed { i, field ->
                if (field.field.declaringClass == info) {
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
                    |    ClassState state;
                    |    JAVA_LONG initThread;
                    |    JavaClassInfo *info;
                    |
                    |    JAVA_OBJECT classLoader;
                    |    JAVA_OBJECT classInstance;
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
                    |    ${it.cStorageType} ${it.cName};
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
                    |${it.cDeclaration};    // ${clazz.className}.${it.name}${it.descriptor}
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
                    |#include "vm_thread.h"
                    |#include "vm_bytecode.h"
                    |
                    |#include "${ClassInfoWriter.headerNameOf(isRt)}"
                    |
                    |""".trimMargin()
        )
        // Include super classes
        info.visitSuperClasses(object : ClassHandler{
            override fun handleAnyClass(clazz: Clazz) {
                cWriter.write(
                    """
                    |#include "_${clazz.requireClassInfo().cIdentifier}.h"
                    |""".trimMargin()
                )
            }
        })

        // Write declarations
        cWriter.write(
            """
                    |
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
                    |    &${it.classInfo!!.cName},
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
                    it.cName
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
                val dc = if (it.field.declaringClass == info) {
                    CNull
                } else {
                    "&${it.field.declaringClass.cName}"
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
                    |    .clinit = ${info.clinit?.cName ?: CNull},
                    |
                    |    .finalizer = ${info.finalizer?.cName ?: CNull},
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
                    |    clazz->staticFields[$i].offset = offsetof(${info.cClassName}, ${field.cName});
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

        // Set static const field to its initial value, except Strings
        val noneStringStaticFieldsWithDefault = info.preResolvedStaticFields.filter { f ->
            f.field.defaultValue != null && f.field.defaultValue !is String
        }
        if (noneStringStaticFieldsWithDefault.isNotEmpty()) {
            cWriter.write(
                """
                    |    // Init default value for static fields, except Strings
                    |""".trimMargin()
            )
            noneStringStaticFieldsWithDefault.forEach { f ->
                cWriter.write(
                    """
                    |    clazz->${f.cName} = ${(f.field.defaultValue as Number).toCConst()};
                    |""".trimMargin()
                )
            }
        }

        cWriter.write(
            """
                    |}
                    |
                    |""".trimMargin()
        )

        // Write implementation of concrete methods
        val nonAbstractMethods = info.methods.withIndex().filterNot { it.value.isAbstract }
        if (nonAbstractMethods.isNotEmpty()) {
            cWriter.write(
                """
                    |// Method implementations
                    |
                    |""".trimMargin()
            )

            nonAbstractMethods.forEach {
                when {
                    it.value.isConcrete -> writeMethodImpl(cWriter, info, it.index, it.value)
                    it.value.isNative -> writeNativeMethodBridge(cWriter, info, it.index, it.value)
                    else -> LOGGER.error(
                        "Unable to generate method impl for {}.{}{}: unexpected method type",
                        clazz.className,
                        it.value.name,
                        it.value.descriptor
                    )
                }
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
                    |// ${clazz.className}.${method.name}${method.descriptor}
                    |${method.cDeclaration} {
                    |    $stackStartStatement;
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
                    |    bc_prepare_arguments(${argumentCount});
                    |""".trimMargin()
            )
        }
        if(!method.isStatic) {
            cWriter.write(
                """
                    |    bc_check_objectref();
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
                    |    bc_label(${inst.index(method)});
                    |""".trimMargin()
                    )
                }
                is LineNumberNode -> {
                    cWriter.write(
                        """
                    |    bc_line(${inst.line}); // Line ${inst.line}
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
                    |    ${functionName}(${inst.`var`});
                    |""".trimMargin()
                            )
                        }
                        else -> throw IllegalArgumentException("Unexpected opcode ${inst.opcode}")
                    }
                }
                is IincInsnNode -> {
                    cWriter.write(
                        """
                    |    bc_iinc(${inst.`var`}, ${inst.incr});
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
                    |    ${functionName}(${inst.operand});
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
                    |    ${functionName}();
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
                    |    ${functionName}(${inst.label.cName(method)});
                    |""".trimMargin()
                            )
                        }
                        else -> throw IllegalArgumentException("Unexpected opcode ${inst.opcode}")
                    }
                }
                is TableSwitchInsnNode -> {
                    writeSwitch(cWriter, method, (inst.min..inst.max).toList(), inst.labels, inst.dflt)
                }
                is LookupSwitchInsnNode -> {
                    writeSwitch(cWriter, method, inst.keys, inst.labels, inst.dflt)
                }
                is LdcInsnNode -> {
                    when(val v = inst.cst) {
                        is Int -> {
                            cWriter.write(
                                """
                    |    bc_ldc_int(${v.toCConst()});
                    |""".trimMargin()
                            )
                        }
                        is Long -> {
                            cWriter.write(
                                """
                    |    bc_ldc_long(${v.toCConst()});
                    |""".trimMargin()
                            )
                        }
                        is Float -> {
                            cWriter.write(
                                """
                    |    bc_ldc_float(${v.toCConst()});
                    |""".trimMargin()
                            )
                        }
                        is Double -> {
                            cWriter.write(
                                """
                    |    bc_ldc_double(${v.toCConst()});
                    |""".trimMargin()
                            )
                        }
                        else -> {
                            // TODO
                        }
                    }
                }
                is FieldInsnNode -> {
                    // first we resolve the field
                    // To resolve an unresolved symbolic reference from D to a field in a class or interface
                    // C, the symbolic reference to C given by the field reference must first be resolved
                    // (§5.4.3.1).
                    val ownerClass = requireNotNull(classPool.getClass(inst.owner)) {
                        "Could not find class ${inst.owner}"
                    }
                    if (!clazzInfo.canAccess(ownerClass.requireClassInfo())) {
                        throw IllegalAccessError("tried to access class $ownerClass from class $clazzInfo")
                    }

                    // jvms8 §5.4.3.2 Field Resolution
                    // When resolving a field reference, field resolution first attempts to look up the
                    // referenced field in C and its superclasses
                    val resolvedField = ownerClass.requireClassInfo().fieldLookup(inst.name, inst.desc)
                    // Then:
                    // • If field lookup fails, field resolution throws a NoSuchFieldError.
                    if (resolvedField == null) {
                        throw NoSuchFieldError("Unable to find field ${inst.name} from class $ownerClass and its super classes")
                    }
                    // • Otherwise, if field lookup succeeds but the referenced field is not accessible
                    // (§5.4.4) to D, field resolution throws an IllegalAccessError.
                    if (!clazzInfo.canAccess(resolvedField)) {
                        throw IllegalAccessError("tried to access field $resolvedField from class $clazzInfo")
                    }
                    // Here we ignore the loading constraint

                    when (inst.opcode) {
                        Opcodes.GETSTATIC -> {
                            // if the resolved field is not a static
                            // (class) field or an interface field, getstatic throws an
                            // IncompatibleClassChangeError.
                            if (!resolvedField.isStatic) {
                                throw IncompatibleClassChangeError("$resolvedField is not a static field")
                            }
                        }
                        Opcodes.PUTSTATIC -> {
                            // if the resolved field is not a static
                            // (class) field or an interface field, putstatic throws an
                            // IncompatibleClassChangeError.
                            if (!resolvedField.isStatic) {
                                throw IncompatibleClassChangeError("$resolvedField is not a static field")
                            }

                            // Otherwise, if the field is final, it must be declared in the current
                            // class, and the instruction must occur in the <clinit> method of
                            // the current class. Otherwise, an IllegalAccessError is thrown.
                            if (resolvedField.isFinal) {
                                if (resolvedField.declaringClass != clazzInfo || !method.isClassInitializer) {
                                    throw IllegalAccessError("tried to write a static final field $resolvedField outside the <clinit> of current class")
                                }
                            }
                        }
                        Opcodes.GETFIELD -> {
                            // if the resolved field is a static field, getfield throws
                            // an IncompatibleClassChangeError.
                            if (resolvedField.isStatic) {
                                throw IncompatibleClassChangeError("$resolvedField is not a instance field")
                            }
                        }
                        Opcodes.PUTFIELD -> {
                            // if the resolved field is a static field, putfield throws
                            // an IncompatibleClassChangeError.
                            if (resolvedField.isStatic) {
                                throw IncompatibleClassChangeError("$resolvedField is not a instance field")
                            }

                            // Otherwise, if the field is final, it must be declared in the
                            // current class, and the instruction must occur in an instance
                            // initialization method (<init>) of the current class. Otherwise, an
                            // IllegalAccessError is thrown.
                            if (resolvedField.isFinal) {
                                if (resolvedField.declaringClass != clazzInfo || !method.isConstructor) {
                                    throw IllegalAccessError("tried to write a final field $resolvedField outside the <init> of current class")
                                }
                            }
                        }
                        else -> throw IllegalArgumentException("Unexpected opcode ${inst.opcode}")
                    }
                }
                is MethodInsnNode -> {
                    // first we resolve the method
                    if (inst.owner.startsWith("[") || inst.owner == "java/lang/invoke/SerializedLambda") {
                        // TODO: handle array method
                    } else {
                        // To resolve an unresolved symbolic reference from D to a method in a class C, the
                        // symbolic reference to C given by the method reference is first resolved (§5.4.3.1).
                        val ownerClass = requireNotNull(classPool.getClass(inst.owner)) {
                            "Could not find class ${inst.owner}"
                        }
                        if (!clazzInfo.canAccess(ownerClass.requireClassInfo())) {
                            throw IllegalAccessError("tried to access class $ownerClass from class $clazzInfo")
                        }
                        if (ownerClass.requireClassInfo().isInterface != inst.itf) {
                            throw IncompatibleClassChangeError()
                        }
                        // jvms8 §5.4.3.3 Method Resolution and §5.4.3.4 Interface Method Resolution
                        val resolvedMethod = ownerClass.requireClassInfo().methodLookup(inst.name, inst.desc)
                        // Then:
                        // • If method lookup fails, method resolution throws a NoSuchMethodError.
                        if (resolvedMethod == null) {
                            throw NoSuchMethodError("Unable to find method ${inst.name} from class $ownerClass and its super classes")
                        }
                        // • Otherwise, if method lookup succeeds and the referenced method is not
                        //   accessible (§5.4.4) to D, method resolution throws an IllegalAccessError.
                        if (!clazzInfo.canAccess(resolvedMethod)) {
                            throw IllegalAccessError("tried to access method $resolvedMethod from class $clazzInfo")
                        }
                        // TODO: All the class names mentioned in the descriptor are resolved (§5.4.3.1).
                        // Here we ignore the loading constraint

                        when (inst.opcode) {
                            Opcodes.INVOKEVIRTUAL -> {
                                // if the resolved method is a class
                                // (static) method, the invokevirtual instruction throws an
                                // IncompatibleClassChangeError.
                                if (resolvedMethod.isStatic) {
                                    throw IncompatibleClassChangeError("$resolvedMethod is not a instance method")
                                }
                            }
                            Opcodes.INVOKESPECIAL -> {
                                // if the resolved method is an instance initialization
                                // method, and the class in which it is declared is not the class
                                // symbolically referenced by the instruction, a NoSuchMethodError
                                // is thrown.
                                if (resolvedMethod.isConstructor) {
                                    if (resolvedMethod.declaringClass != ownerClass.requireClassInfo()) {
                                        throw NoSuchMethodError("Unable to find method ${inst.name} from class $ownerClass")
                                    }
                                }

                                // if the resolved method is a class
                                // (static) method, the invokespecial instruction throws an
                                // IncompatibleClassChangeError.
                                if (resolvedMethod.isStatic) {
                                    throw IncompatibleClassChangeError("$resolvedMethod is not a instance method")
                                }

                                writeInvokeSpecial(cWriter, clazzInfo, ownerClass.requireClassInfo(), resolvedMethod)
                            }
                            Opcodes.INVOKESTATIC -> {
                                // if the resolved method is an instance
                                // method, the invokestatic instruction throws an
                                // IncompatibleClassChangeError.
                                if (!resolvedMethod.isStatic) {
                                    throw IncompatibleClassChangeError("$resolvedMethod is not a static method")
                                }
                            }
                            Opcodes.INVOKEINTERFACE -> {
                                // if the resolved method is static or
                                // private, the invokeinterface instruction throws an
                                // IncompatibleClassChangeError.
                                if (resolvedMethod.isStatic) {
                                    throw IncompatibleClassChangeError("$resolvedMethod is not a instance method")
                                }
                                if (resolvedMethod.isPrivate) {
                                    throw IncompatibleClassChangeError("$resolvedMethod is a private method")
                                }
                            }
                            else -> throw IllegalArgumentException("Unexpected opcode ${inst.opcode}")
                        }
                    }
                }
            }
        }

        if(method.descriptor.returnType.sort != Type.VOID) {
            // TODO: throw exception if control flow reaches here
            cWriter.write(
                """
                    |    return 0;
                    |""".trimMargin()
            )
        }

        cWriter.write(
            """
                    |}
                    |
                    |""".trimMargin()
        )
    }

    private fun writeSwitch(
        cWriter: Writer,
        method: MethodInfo,
        keys: List<Int>,
        branches: List<LabelNode>,
        default: LabelNode
    ) {
        assert(keys.size == branches.size) {
            "switch: key count (${keys.size}) != branch count (${branches.size})"
        }

        cWriter.write(
            """
                    |    bc_switch() {
                    |""".trimMargin()
        )
        keys.zip(branches).forEach { (k, label) ->
            cWriter.write(
                """
                    |        case ${k.toCConst()}: goto ${label.cName(method)};
                    |""".trimMargin()
            )
        }
        cWriter.write(
            """
                    |        default: goto ${default.cName(method)};
                    |    }
                    |""".trimMargin()
        )
    }

    private fun writeNativeMethodBridge(cWriter: Writer, clazzInfo: ClassInfo, index: Int, method: MethodInfo) {
        val clazz = clazzInfo.thisClass
        val node = method.methodNode

        // For native method, `maxStack` and `maxLocals` are always 0
        // here we use the parameter count as the `maxLocals` so we can pass parameters
        // like a normal method.
        val argumentTypes = method.descriptor.argumentTypes
        val argumentCount = if (method.isStatic) {
            argumentTypes.size
        } else {
            argumentTypes.size + 1 // implicitly passed this
        }

        val stackStartStatement = if(argumentCount == 0) {
            "stack_frame_start_zero(${index})"
        } else {
            "stack_frame_start(${index}, 0 /* native method does not use stack */, $argumentCount)"
        }
        cWriter.write(
            """
                    |// Native method bridge for ${clazz.className}.${method.name}${method.descriptor}
                    |${method.cDeclaration} {
                    |    $stackStartStatement;
                    |""".trimMargin()
        )

        // Pass arguments
        if (argumentCount > 0) {
            cWriter.write(
                """
                    |    bc_prepare_arguments(${argumentCount});
                    |""".trimMargin()
            )
        }

        // TODO: resolve the function pointer to the native method
        cWriter.write(
            """
                |
                |    // Resolve the function ptr
                |    ${Jni.declFunctionPtr("ptr", method.isStatic, method.descriptor.returnType, argumentTypes.toList())};
                |    ptr = NULL; // TODO
                |
                |""".trimMargin()
        )

        // Call real function
        val hasReturnValue = method.descriptor.returnType.sort != Type.VOID
        if (hasReturnValue) {
            cWriter.write(
                """
                |    // Call real function
                |    ${method.descriptor.returnType.toJNIType()} result = ptr(
                |""".trimMargin()
            )
        } else {
            cWriter.write(
                """
                |    // Call real function
                |    ptr(
                |""".trimMargin()
            )
        }
        // Build argument list
        val jniArgs = mutableListOf<String>()
        // Pass JNIEnv
        jniArgs.add("NULL") // TODO: pass env
        // Pass second param
        if(method.isStatic) {
            // Pass current class
            jniArgs.add("(${Jni.TYPE_CLASS}) THIS_CLASS")
        } else {
            // $this is stored in the local[0]
            jniArgs.add("bc_jni_arg_jref(0, ${Jni.TYPE_OBJECT})")
        }
        // Pass remaining arguments
        val localOffset = if (method.isStatic) {
            0
        } else {
            1
        }
        argumentTypes.withIndex().mapTo(jniArgs) { (index, arg) ->
            when (arg.sort) {
                Type.BOOLEAN,
                Type.CHAR,
                Type.BYTE,
                Type.SHORT,
                Type.INT,
                Type.FLOAT,
                Type.LONG,
                Type.DOUBLE -> "bc_jni_arg_${arg.toJNIType()}(${index + localOffset})"

                Type.ARRAY,
                Type.OBJECT -> "bc_jni_arg_jref(${index + localOffset}, ${arg.toJNIType()})"

                else -> throw IllegalArgumentException("Unexpected type ${arg.sort} for argument.")
            }
        }
        val jniArgStr = jniArgs.joinToString(separator = """
                |,
                |        """.trimMargin())
        cWriter.write(
            """
                |        $jniArgStr
                |    );
                |
                |""".trimMargin()
        )

        // End the frame
        cWriter.write(
            """
                    |    stack_frame_end();
                    |""".trimMargin()
        )

        // Handle return
        if(hasReturnValue) {
            cWriter.write(
                """
                    |
                    |    return result;
                    |""".trimMargin()
            )
        }

        cWriter.write(
            """
                    |}
                    |
                    |""".trimMargin()
        )
    }

    /** jvms8 §6.5 Instructions - invokespecial */
    private fun writeInvokeSpecial(cWriter: Writer, currentClass: ClassInfo, ownerClass: ClassInfo, resolvedMethod: MethodInfo) {
        // If all of the following are true, let C be the direct superclass of the
        // current class:
        @Suppress("SimplifyBooleanWithConstants")
        val classToLookup: ClassInfo = if (
            // • The resolved method is not an instance initialization method (§2.9).
            !resolvedMethod.isConstructor
            // • If the symbolic reference names a class (not an interface), then
            //   that class is a superclass of the current class.
            && (!ownerClass.isInterface && ownerClass superclassOf currentClass)
            // • The ACC_SUPER flag is set for the class file (§4.1).
            // jvms8 §4.1 The ClassFile Structure
            // In Java SE 8 and above, the Java
            // Virtual Machine considers the ACC_SUPER flag to be set in every class file,
            // regardless of the actual value of the flag in the class file and the version of
            // the class file.
            && true
        ) {
            currentClass.superClass!!.requireClassInfo()
        } else {
            // Otherwise, let C be the class or interface named by the symbolic
            // reference.
            ownerClass
        }

        // The actual method to be invoked is selected by the following
        // lookup procedure:
        // 1. If C contains a declaration for an instance method with the
        //    same name and descriptor as the resolved method, then it is
        //    the method to be invoked.
        val methodToInvoke: MethodInfo = (classToLookup.findDeclMethod(resolvedMethod.name, resolvedMethod.descriptor.descriptor)
            ?.takeIf { !it.isStatic }
            ?: if (!classToLookup.isInterface) {
                // 2. Otherwise, if C is a class and has a superclass, a search for
                //    a declaration of an instance method with the same name
                //    and descriptor as the resolved method is performed, starting
                //    with the direct superclass of C and continuing with the direct
                //    superclass of that class, and so forth, until a match is found or
                //    no further superclasses exist. If a match is found, then it is the
                //    method to be invoked.
                fun ClassInfo.methodLookupInSuperclass(): MethodInfo? {
                    return superClass?.requireClassInfo()?.let { superClazz ->
                        superClazz.findDeclMethod(resolvedMethod.name, resolvedMethod.descriptor.descriptor)
                            ?.takeIf { !it.isStatic }
                            ?: superClazz.methodLookupInSuperclass()
                    }
                }
                classToLookup.methodLookupInSuperclass()
            } else {
                // 3. Otherwise, if C is an interface and the class Object contains a
                //    declaration of a public instance method with the same name
                //    and descriptor as the resolved method, then it is the method
                //    to be invoked.
                val objectClass = requireNotNull(classPool.getClass(Clazz.CLASS_JAVA_LANG_OBJECT)) {
                    "cannot find class ${Clazz.CLASS_JAVA_LANG_OBJECT}"
                }.requireClassInfo()
                objectClass.findDeclMethod(resolvedMethod.name, resolvedMethod.descriptor.descriptor)
                    ?.takeIf { it.isPublic && !it.isStatic }
            })?.also {
            // if step 1, step 2, or step 3 of the lookup
            // procedure selects an abstract method, invokespecial throws an
            // AbstractMethodError.
            if (it.isAbstract) {
                throw AbstractMethodError()
            }
        } ?: run {
            // 4. Otherwise, if there is exactly one maximally-specific method
            //    (§5.4.3.3) in the superinterfaces of C that matches the resolved
            //    method's name and descriptor and is not abstract, then it is
            //    the method to be invoked.
            val maxSpecMethods = classToLookup.findMaximallySpecificSuperInterfaceMethod(
                resolvedMethod.name,
                resolvedMethod.descriptor.descriptor
            )
                .filter { !it.isAbstract }
            when (maxSpecMethods.size) {
                0 -> {
                    // Otherwise, if step 4 of the lookup procedure determines there
                    // are zero maximally-specific methods in the superinterfaces of C
                    // that match the resolved method's name and descriptor and are not
                    // abstract, invokespecial throws an AbstractMethodError.
                    throw AbstractMethodError()
                }
                1 -> {
                    maxSpecMethods.single()
                }
                else -> {
                    // Otherwise, if step 4 of the lookup procedure determines
                    // there are multiple maximally-specific methods in the
                    // superinterfaces of C that match the resolved method's name
                    // and descriptor and are not abstract, invokespecial throws an
                    // IncompatibleClassChangeError
                    throw IncompatibleClassChangeError()
                }
            }
        }

        // Here we successfully resolved and found the target method to invoke,
        // with only one rule that has not been checked:
        // > If the resolved method is protected, and it is a member of a
        // > superclass of the current class, and the method is not declared in
        // > the same run-time package (§5.3) as the current class, then the
        // > class of objectref must be either the current class or a subclass of
        // > the current class.
        // Which cannot be checked until runtime, so we don't worry about it here

        // Generate opcode instruction that do the invokespecial
        cWriter.write(
            """
                    |    // invokespecial ${resolvedMethod.declaringClass.thisClass.className}.${resolvedMethod.name}${resolvedMethod.descriptor}
                    |    bc_invoke_special(${methodToInvoke.cName});
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
