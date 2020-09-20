package io.noisyfox.foxvm.translator.cgen

import io.noisyfox.foxvm.bytecode.ClassPool
import io.noisyfox.foxvm.bytecode.asCString
import io.noisyfox.foxvm.bytecode.clazz.ClassInfo
import io.noisyfox.foxvm.bytecode.clazz.Clazz
import io.noisyfox.foxvm.bytecode.clazz.MethodInfo
import io.noisyfox.foxvm.bytecode.clazz.PreResolvedFieldInfo
import io.noisyfox.foxvm.bytecode.visitor.ClassHandler
import io.noisyfox.foxvm.translator.DiffFileWriter
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
import org.objectweb.asm.tree.TypeInsnNode
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
    private val constantPool: StringConstantPool,
    private val outputDir: File
) : ClassHandler {

    var processedClassNumber: Int = 0
        private set

    var updatedClassNumber: Int = 0
        private set

    var processedMethodNumber: Long = 0
        private set

    var processedInstructionNumber: Long = 0
        private set

    override fun handleApplicationClass(clazz: Clazz) {
        processedClassNumber++

        val info = clazz.requireClassInfo()

        // Generate header file
        val headerUpdated = DiffFileWriter(File(outputDir, "_${info.cIdentifier}.h")).use {
            it.buffered().use { headerWriter ->
                writeH(headerWriter, clazz, info)
            }
            it.didUpdateFile
        }

        // Generate C file
        val cUpdated = DiffFileWriter(File(outputDir, "_${info.cIdentifier}.c")).use {
            CWriter(it.buffered()).use { cWriter ->
                writeC(cWriter, clazz, info)
            }
            it.didUpdateFile
        }

        if (headerUpdated || cUpdated) {
            updatedClassNumber++
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

        // Write the reference to class info
        headerWriter.write(
            """
                    |extern JavaClassInfo ${info.cName};
                    |
                    |""".trimMargin()
        )

        // Generate class definition
        headerWriter.write(
            """
                    |// Class definition
                    |typedef struct _${info.cClassName} {
                    |    JavaClass baseClass;
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
                    |    ResolvedField $CNAME_BACKING_STATIC_FIELDS[${info.preResolvedStaticFields.size}];
                    |""".trimMargin()
            )
        }
        if (info.preResolvedInstanceFields.isNotEmpty()) {
            headerWriter.write(
                """
                    |    ResolvedField $CNAME_BACKING_INSTANCE_FIELDS[${info.preResolvedInstanceFields.size}];
                    |""".trimMargin()
            )
        }

        // Write static field data storage
        if (info.preResolvedStaticFields.isNotEmpty()) {
            headerWriter.write(
                """
                    |
                    |    // Start of static field storage
                    |""".trimMargin()
            )
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
                    |    JavaObjectBase baseObject;
                    |""".trimMargin()
        )

        if (info.preResolvedInstanceFields.isNotEmpty()) {
            headerWriter.write(
                """
                    |
                    |    // Start of instance field storage
                    |""".trimMargin()
            )
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
                    |${it.cFunctionDeclaration};    // ${clazz.className}.${it.name}${it.descriptor}
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

    private fun writeC(cWriter: CWriter, clazz: Clazz, info: ClassInfo) {
        cWriter.write(
            """
                    |// Generated from class [${clazz.className}] at ${clazz.filePath}
                    |
                    |""".trimMargin()
        )

        // Write includes
        cWriter.write(
            """
                    |#include "vm_thread.h"
                    |#include "vm_bytecode.h"
                    |#include "vm_string.h"
                    |
                    |""".trimMargin()
        )
        // Include dependencies here
        cWriter.includeDependenciesHere()

        // Add self as dependency
        cWriter.addDependency(info)

        // Add superclass as dependency
        info.superClass?.requireClassInfo()?.also {
            cWriter.addDependency(it)
        }

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
                // Add dependency
                cWriter.addDependency(it.requireClassInfo())
                cWriter.write(
                    """
                    |    &${it.requireClassInfo().cName},
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

        fun String?.constantID(): String = if(this == null) {
            "STRING_CONSTANT_NULL"
        } else {
            constantPool.addConstant(this).toString()
        }

        // Write field info
        if (info.fields.isNotEmpty()) {
            cWriter.write(
                """
                    |// Fields of this class
                    |static FieldInfo ${info.cNameFields}[] = {
                    |""".trimMargin()
            )

            info.fields.forEach { f ->
                val offset = if (f.isStatic) {
                    val resolvedStaticFieldInfo = info.preResolvedStaticFields.single { it.field == f }
                    "offsetof(${info.cClassName}, ${resolvedStaticFieldInfo.cName})"
                } else {
                    val resolvedInstanceFieldInfo = info.preResolvedInstanceFields.single { it.field == f }
                    "offsetof(${info.cObjectName}, ${resolvedInstanceFieldInfo.cName})"
                }

                val defaultStringVal: String? = if (f.isStatic && f.defaultValue is String) {
                    if(f.descriptor.internalName != Clazz.CLASS_JAVA_LANG_STRING) {
                        throw IncompatibleClassChangeError("Non-string field $f cannot have string default value")
                    }
                    f.defaultValue
                } else {
                    null
                }

                cWriter.write(
                    """
                    |    {
                    |        .accessFlags = ${AccFlag.translateFieldAcc(f.access)},
                    |        .name = ${f.name.constantID()}, // "${f.name.asCString()}"
                    |        .descriptor = ${f.descriptor.toString().constantID()}, // "${f.descriptor.toString().asCString()}"
                    |        .signature = ${f.signature.constantID()}, // ${f.signature.toCString()}
                    |        .offset = $offset,
                    |        .defaultConstantIndex = ${defaultStringVal.constantID()},
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
                    |""".trimMargin()
            )

            info.methods.forEach {
                val codeRef = if (it.isAbstract) {
                    CNull
                } else {
                    it.cFunctionName
                }
                if(it.isNative) {
                    cWriter.write(
                        """
                    |static MethodInfoNative ${it.cName} = {
                    |        .method = {
                    |                .accessFlags = ${AccFlag.translateMethodAcc(it.access)},
                    |                .name = ${it.name.constantID()}, // "${it.name.asCString()}"
                    |                .descriptor = ${it.descriptor.toString().constantID()}, // "${it.descriptor.toString().asCString()}"
                    |                .signature = ${it.signature.constantID()}, // ${it.signature.toCString()}
                    |                .declaringClass = &${info.cName},
                    |                .code = $codeRef,
                    |        },
                    |        .shortName = "${it.jniShortName}",
                    |        .longName  = "${it.jniLongName}",
                    |        .nativePtr = $CNull,
                    |};
                    |""".trimMargin()
                    )
                } else {
                    cWriter.write(
                        """
                    |static MethodInfo ${it.cName} = {
                    |        .accessFlags = ${AccFlag.translateMethodAcc(it.access)},
                    |        .name = ${it.name.constantID()}, // "${it.name.asCString()}"
                    |        .descriptor = ${it.descriptor.toString().constantID()}, // "${it.descriptor.toString().asCString()}"
                    |        .signature = ${it.signature.constantID()}, // ${it.signature.toCString()}
                    |        .declaringClass = &${info.cName},
                    |        .code = $codeRef,
                    |};
                    |""".trimMargin()
                    )
                }
            }

            cWriter.write(
                """
                    |
                    |static MethodInfo* ${info.cNameMethods}[] = {
                    |""".trimMargin()
            )

            info.methods.forEach {
                cWriter.write(
                    """
                    |    (MethodInfo*)&${it.cName},
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

        fun CWriter.write(f: PreResolvedFieldInfo) {
            val dc = if (f.field.declaringClass == info) {
                CNull
            } else {
                // Add dependency
                addDependency(f.field.declaringClass)
                "&${f.field.declaringClass.cName}"
            }
            write(
                """
                    |    {
                    |        .declaringClass = $dc,
                    |        .fieldIndex = ${f.fieldIndex},
                    |        .isReference = ${f.isReference.translate()},
                    |    },
                    |""".trimMargin()
            )
        }

        // Write pre-resolved static field info
        if (info.preResolvedStaticFields.isNotEmpty()) {
            cWriter.write(
                """
                    |// Pre-resolved static fields of this class
                    |static PreResolvedFieldInfo ${info.cNameStaticFields}[] = {
                    |""".trimMargin()
            )

            info.preResolvedStaticFields.forEach {
                cWriter.write(it)
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
                    |static PreResolvedFieldInfo ${info.cNameInstanceFields}[] = {
                    |""".trimMargin()
            )

            info.preResolvedInstanceFields.forEach {
                cWriter.write(it)
            }

            cWriter.write(
                """
                    |};
                    |
                    |""".trimMargin()
            )
        }

        // Write vtable
        if (info.vtable.isNotEmpty()) {
            cWriter.write(
                """
                    |// VTable of this class
                    |static VTableItem ${info.cNameVTable}[] = {
                    |""".trimMargin()
            )

            info.vtable.forEach {
                val dc = if (it.declaringClass == info) {
                    CNull
                } else {
                    // Add dependency
                    cWriter.addDependency(it.declaringClass)
                    "&${it.declaringClass.cName}"
                }
                val codeRef = if (it.isAbstract) {
                    CNull
                } else {
                    it.cFunctionName
                }
                cWriter.write(
                    """
                    |    { // $it
                    |        .declaringClass = $dc,
                    |        .methodIndex = ${it.declaringClass.methods.indexOf(it)},
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

        // Write IVTable
        if (info.ivtable.isNotEmpty()) {
            cWriter.write(
                """
                    |// IVTable of this class
                    |""".trimMargin()
            )

            // Write method indexes first
            info.ivtable.forEach { ivti ->
                cWriter.write(
                    """
                    |static IVTableMethodIndex ${ivti.cNameMethodIndex(info)}[] = {
                    |""".trimMargin()
                )

                ivti.methodIndexes.forEach { mi ->
                    val m = ivti.declaringInterface.methods[mi.methodIndex]

                    cWriter.write(
                        """
                    |    { // $m
                    |        .methodIndex = ${mi.methodIndex},
                    |        .vtableIndex = ${mi.vtableIndex},
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

            // Then write ivtable
            cWriter.write(
                """
                    |static IVTableItem ${info.cNameIVTable}[] = {
                    |""".trimMargin()
            )

            info.ivtable.forEach { ivti ->
                // Add dependency
                cWriter.addDependency(ivti.declaringInterface)

                cWriter.write(
                    """
                    |    { // ${ivti.declaringInterface}
                    |        .declaringInterface = &${ivti.declaringInterface.cName},
                    |        .indexCount = ${ivti.methodIndexes.size},
                    |        .methodIndexes = ${ivti.cNameMethodIndex(info)},
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

        // Write class info
        cWriter.write(
            """
                    |// Class info
                    |JavaClassInfo ${info.cName} = {
                    |    .accessFlags = ${AccFlag.translateClassAcc(clazz.access)},
                    |    .modifierFlags = ${AccFlag.translateClassAcc(info.modifier)},
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
                    |    .vtableCount = ${info.vtable.size},
                    |    .vtable = ${info.cNameVTable},
                    |
                    |    .ivtableCount = ${info.ivtable.size},
                    |    .ivtable = ${info.cNameIVTable},
                    |
                    |    .clinit = ${info.clinit?.cFunctionName ?: CNull},
                    |
                    |    .finalizer = ${info.finalizer?.cFunctionName ?: CNull},
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
                    |    c->interfaces = clazz->$CNAME_BACKING_INTERFACES;
                    |""".trimMargin()
            )
        } else {
            cWriter.write(
                """
                    |    c->interfaces = $CNull;
                    |""".trimMargin()
            )
        }

        // Resolve static fields
        if (info.preResolvedStaticFields.isNotEmpty()) {
            cWriter.write(
                """
                    |    c->staticFields = clazz->$CNAME_BACKING_STATIC_FIELDS;
                    |""".trimMargin()
            )
        } else {
            cWriter.write(
                """
                    |    c->staticFields = $CNull;
                    |""".trimMargin()
            )
        }

        // Resolve instance fields
        if (info.preResolvedInstanceFields.isNotEmpty()) {
            cWriter.write(
                """
                    |    c->fields = clazz->$CNAME_BACKING_INSTANCE_FIELDS;
                    |""".trimMargin()
            )
        } else {
            cWriter.write(
                """
                    |    c->fields = $CNull;
                    |""".trimMargin()
            )
        }

        // Set static const field to its initial value, except Strings
        val noneStringStaticFieldsWithDefault = info.preResolvedStaticFields.filter { f ->
            f.field.defaultValue != null && f.field.defaultValue !is String
        }
        if (noneStringStaticFieldsWithDefault.isNotEmpty()) {
            cWriter.write(
                """
                    |
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
        val nonAbstractMethods = info.methods.filterNot { it.isAbstract }
        if (nonAbstractMethods.isNotEmpty()) {
            cWriter.write(
                """
                    |// Method implementations
                    |
                    |""".trimMargin()
            )

            nonAbstractMethods.forEach {
                processedMethodNumber++

                when {
                    it.isConcrete -> writeMethodImpl(cWriter, info, it)
                    it.isNative -> {
                        if(it.isFastNative) {
                            writeFastNativeMethodBridge(cWriter, info, it)
                        } else {
                            writeNativeMethodBridge(cWriter, info, it)
                        }
                    }
                    else -> LOGGER.error(
                        "Unable to generate method impl for {}.{}{}: unexpected method type",
                        clazz.className,
                        it.name,
                        it.descriptor
                    )
                }
            }
        }
    }

    private fun writeMethodImpl(cWriter: CWriter, clazzInfo: ClassInfo, method: MethodInfo) {
        val clazz = clazzInfo.thisClass
        val node = method.methodNode

        val stackStartStatement = if(node.maxStack == 0 && node.maxLocals == 0) {
            "stack_frame_start_zero(&${method.cName})"
        } else {
            "stack_frame_start(&${method.cName}, ${node.maxStack}, ${node.maxLocals})"
        }
        cWriter.write(
            """
                    |// ${clazz.className}.${method.name}${method.descriptor}
                    |${method.cFunctionDeclaration} {
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

        // Define exception handlers
        if (node.tryCatchBlocks.isNotEmpty()) {
            cWriter.write(
                """
                    |
                    |    exception_frame_start();
                    |""".trimMargin()
            )

            val throwableClass = requireNotNull(classPool.getClass("java/lang/Throwable")) {
                "cannot find class java/lang/Throwable"
            }.requireClassInfo()

            node.tryCatchBlocks.forEach {
                if(it.type == null) {
                    cWriter.write(
                        """
                    |        exception_new_block(${it.start.index(method)}, ${it.end.index(method)}, ${it.handler.cName(method)}, $CNull);
                    |""".trimMargin()
                    )
                } else {
                    val exceptionClass = requireNotNull(classPool.getClass(it.type)) {
                        "cannot find class ${it.type}"
                    }.requireClassInfo()

                    require(exceptionClass instanceOf throwableClass) {
                        "class $exceptionClass is not a subclass of $throwableClass"
                    }

                    // Add the class as dependency
                    cWriter.addDependency(exceptionClass)

                    cWriter.write(
                        """
                    |        exception_new_block(${it.start.index(method)}, ${it.end.index(method)}, ${it.handler.cName(method)}, &${exceptionClass.cName});
                    |""".trimMargin()
                    )
                }
            }

            cWriter.write(
                """
                    |        exception_not_handled();
                    |    exception_block_end();
                    |""".trimMargin()
            )
        } else if(method.isSynchronized) {
            // Need an empty try for releasing the lock before exception is thrown up
            cWriter.write(
                """
                    |
                    |    // Empty exception frame for freeing the lock of synchronized method
                    |    exception_frame_start();
                    |        exception_not_handled();
                    |    exception_block_end();
                    |""".trimMargin()
            )
        }

        // TODO: write instructions

        node.instructions.forEach { inst ->
            processedInstructionNumber++

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
                        Opcodes.RET -> throw IncompatibleClassChangeError("ret not supported")
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
                            when (inst.operand) {
                                in byteCodesNewArrayType -> {
                                    val typeDesc = byteCodesNewArrayType[inst.operand]
                                    cWriter.write(
                                        """
                    |    // newarray ${inst.operand}
                    |    bc_newarray("$typeDesc");
                    |""".trimMargin()
                                    )
                                }
                                else -> throw IllegalArgumentException("Unexpected newarray type ${inst.operand}")
                            }
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
                        in byteCodesReturn -> {
                            val (bytecodesMnemonic, allowedReturnInsn) = byteCodesReturn.getValue(inst.opcode)
                            val returnType = method.descriptor.returnType
                            val functionPrefix = allowedReturnInsn[returnType.sort]
                                ?: throw VerifyError("$bytecodesMnemonic cannot be used in method with return type $returnType")
                            cWriter.write(
                                """
                    |    // $bytecodesMnemonic
                    |    bc_${functionPrefix}return();
                    |""".trimMargin()
                            )
                        }
                        else -> throw IllegalArgumentException("Unexpected opcode ${inst.opcode}")
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
                        is String -> {
                            // Add the string to constant pool
                            val constantPoolIndex = constantPool.addConstant(v)
                            val commentV = if(v.length > 20) {
                                v.take(20) + "..."
                            } else {
                                v
                            }
                            cWriter.write(
                                """
                    |    // ldc "${commentV.asCString()}" (java.lang.String)
                    |    bc_ldc_string($constantPoolIndex);
                    |""".trimMargin()
                            )
                        }
                        is Type -> {
                            when(v.sort) {
                                Type.OBJECT->{
                                    cWriter.write(
                                        """
                    |    // ldc $v
                    |    bc_ldc_class("${v.internalName}");
                    |""".trimMargin()
                                    )
                                }
                                Type.ARRAY -> {
                                    cWriter.write(
                                        """
                    |    // ldc $v
                    |    bc_ldc_class("$v");
                    |""".trimMargin()
                                    )
                                }
                                else -> throw IllegalArgumentException("Unexpected type ${v.sort} for ldc.")
                            }
                        }
                        else -> throw IllegalArgumentException("Unexpected type $v for ldc.")
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

                            // Add the declaring class as dependency
                            cWriter.addDependency(resolvedField.declaringClass)
                            // Get the pre-resolved info
                            val preResolved = resolvedField.declaringClass.preResolvedStaticFields.single { it.field == resolvedField }
                            cWriter.write(
                                """
                    |    // getstatic ${inst.owner}.${inst.name}:${inst.desc}
                    |    bc_getstatic${resolvedField.typeSuffix}(&${resolvedField.declaringClass.cName}, ${resolvedField.declaringClass.cClassName}, ${preResolved.cName});
                    |""".trimMargin()
                            )
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

                            // Add the declaring class as dependency
                            cWriter.addDependency(resolvedField.declaringClass)
                            // Get the pre-resolved info
                            val preResolved = resolvedField.declaringClass.preResolvedStaticFields.single { it.field == resolvedField }
                            cWriter.write(
                                """
                    |    // putstatic ${inst.owner}.${inst.name}:${inst.desc}
                    |    bc_putstatic${resolvedField.typeSuffix}(&${resolvedField.declaringClass.cName}, ${resolvedField.declaringClass.cClassName}, ${preResolved.cName});
                    |""".trimMargin()
                            )
                        }
                        Opcodes.GETFIELD -> {
                            // if the resolved field is a static field, getfield throws
                            // an IncompatibleClassChangeError.
                            if (resolvedField.isStatic) {
                                throw IncompatibleClassChangeError("$resolvedField is not a instance field")
                            }

                            // Add the declaring class as dependency
                            cWriter.addDependency(resolvedField.declaringClass)
                            // Get the pre-resolved info
                            val preResolved = resolvedField.declaringClass.preResolvedInstanceFields.single { it.field == resolvedField }
                            cWriter.write(
                                """
                    |    // getfield ${inst.owner}.${inst.name}:${inst.desc}
                    |    bc_getfield${resolvedField.typeSuffix}(&${resolvedField.declaringClass.cName}, ${preResolved.fieldIndex}, ${resolvedField.declaringClass.cObjectName}, ${preResolved.cName});
                    |""".trimMargin()
                            )
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

                            // Add the declaring class as dependency
                            cWriter.addDependency(resolvedField.declaringClass)
                            // Get the pre-resolved info
                            val preResolved = resolvedField.declaringClass.preResolvedInstanceFields.single { it.field == resolvedField }
                            cWriter.write(
                                """
                    |    // putfield ${inst.owner}.${inst.name}:${inst.desc}
                    |    bc_putfield${resolvedField.typeSuffix}(&${resolvedField.declaringClass.cName}, ${preResolved.fieldIndex}, ${resolvedField.declaringClass.cObjectName}, ${preResolved.cName});
                    |""".trimMargin()
                            )
                        }
                        else -> throw IllegalArgumentException("Unexpected opcode ${inst.opcode}")
                    }
                }
                is MethodInsnNode -> {
                    // first we resolve the method
                    if (inst.owner.startsWith("[")) {
                        if (inst.opcode != Opcodes.INVOKEVIRTUAL) {
                            throw IncompatibleClassChangeError("Method of array type can only be called by invokevirtual (actual: ${inst.opcode})")
                        }
                        if (inst.name != "clone" || inst.desc != "()Ljava/lang/Object;") {
                            LOGGER.error("Array method call: {}.{}{}", inst.owner, inst.name, inst.desc)
                            throw IncompatibleClassChangeError("Cannot invoke ${inst.owner}.${inst.name}${inst.desc}. Only clone()Ljava/lang/Object; can be called on a array class")
                        }

                        // Include array header
                        cWriter.addDependency("vm_array.h")
                        // Use invokespecial here, for simplicity and speed. Should be able to replace to invokevirtual later,
                        // but makes no difference since this method cannot be overridden.
                        cWriter.write(
                            """
                    |    // invokespecial ${inst.owner}.${inst.name}${inst.desc}
                    |    bc_invoke_special_o(g_array_5Mclone_R9Pjava_lang6CObject);
                    |""".trimMargin()
                        )
                    } else {
                        // To resolve an unresolved symbolic reference from D to a method in a class C, the
                        // symbolic reference to C given by the method reference is first resolved (§5.4.3.1).
                        val ownerClass = requireNotNull(classPool.getClass(inst.owner)) {
                            "Could not find class ${inst.owner}"
                        }.requireClassInfo()
                        if (!clazzInfo.canAccess(ownerClass)) {
                            throw IllegalAccessError("tried to access class $ownerClass from class $clazzInfo")
                        }
                        if (ownerClass.isInterface != inst.itf) {
                            throw IncompatibleClassChangeError()
                        }
                        // jvms8 §5.4.3.3 Method Resolution and §5.4.3.4 Interface Method Resolution
                        val resolvedMethod = ownerClass.methodLookup(inst.name, inst.desc)
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

                                // Find the method from the owner class's vtable
                                val vtableIndex = ownerClass.vtable.indexOfFirst { it == resolvedMethod || it overrides resolvedMethod }
                                if (vtableIndex == -1) {
                                    throw IncompatibleClassChangeError("$resolvedMethod not presented in the vtable of class $ownerClass")
                                }
                                val lookupMethod = ownerClass.vtable[vtableIndex]
                                if (lookupMethod != resolvedMethod) {
                                    LOGGER.debug("invokespecial ${inst.owner}.${inst.name}${inst.desc} resolved to $resolvedMethod, but dispatched to $lookupMethod")
                                }

                                val targetMethodArgumentCount = lookupMethod.descriptor.argumentTypes.size + 1 // implicitly passed this
                                cWriter.addDependency(ownerClass)
                                cWriter.write(
                                    """
                    |    // invokevirtual ${inst.owner}.${inst.name}${inst.desc}
                    |    bc_invoke_virtual${lookupMethod.invokeSuffix}($targetMethodArgumentCount, &${ownerClass.cName}, $vtableIndex);
                    |""".trimMargin()
                                )
                            }
                            Opcodes.INVOKESPECIAL -> {
                                // if the resolved method is an instance initialization
                                // method, and the class in which it is declared is not the class
                                // symbolically referenced by the instruction, a NoSuchMethodError
                                // is thrown.
                                if (resolvedMethod.isConstructor) {
                                    if (resolvedMethod.declaringClass != ownerClass) {
                                        throw NoSuchMethodError("Unable to find method ${inst.name} from class $ownerClass")
                                    }
                                }

                                // if the resolved method is a class
                                // (static) method, the invokespecial instruction throws an
                                // IncompatibleClassChangeError.
                                if (resolvedMethod.isStatic) {
                                    throw IncompatibleClassChangeError("$resolvedMethod is not a instance method")
                                }

                                writeInvokeSpecial(
                                    cWriter,
                                    inst,
                                    clazzInfo,
                                    ownerClass,
                                    resolvedMethod
                                )
                            }
                            Opcodes.INVOKESTATIC -> {
                                // if the resolved method is an instance
                                // method, the invokestatic instruction throws an
                                // IncompatibleClassChangeError.
                                if (!resolvedMethod.isStatic) {
                                    throw IncompatibleClassChangeError("$resolvedMethod is not a static method")
                                }

                                // Add the declaring class as dependency
                                cWriter.addDependency(resolvedMethod.declaringClass)
                                cWriter.write(
                                    """
                    |    // invokestatic ${inst.owner}.${inst.name}${inst.desc}
                    |    bc_invoke_static${resolvedMethod.invokeSuffix}(&${resolvedMethod.declaringClass.cName}, ${resolvedMethod.cFunctionName});
                    |""".trimMargin()
                                )
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

                                if (resolvedMethod.declaringClass.thisClass.className == Clazz.CLASS_JAVA_LANG_OBJECT) {
                                    TODO()
                                } else if(!resolvedMethod.declaringClass.isInterface) {
                                    throw IncompatibleClassChangeError("$resolvedMethod is not a interface method")
                                } else {
                                    cWriter.addDependency(resolvedMethod.declaringClass)
                                    val methodIndex = resolvedMethod.declaringClass.methods.indexOf(resolvedMethod)
                                    require(methodIndex >= 0)
                                    val targetMethodArgumentCount = resolvedMethod.descriptor.argumentTypes.size + 1 // implicitly passed this
                                    cWriter.write(
                                        """
                    |    // invokeinterface ${inst.owner}.${inst.name}${inst.desc}
                    |    bc_invoke_interface${resolvedMethod.invokeSuffix}($targetMethodArgumentCount, &${resolvedMethod.declaringClass.cName}, $methodIndex);
                    |""".trimMargin()
                                    )
                                }
                            }
                            else -> throw IllegalArgumentException("Unexpected opcode ${inst.opcode}")
                        }
                    }
                }
                is TypeInsnNode -> {
                    when (inst.opcode) {
                        Opcodes.NEW -> {
                            val resolvedClass = requireNotNull(classPool.getClass(inst.desc)) {
                                "Could not find class ${inst.desc}"
                            }.requireClassInfo()

                            // if the symbolic reference to the class, array, or interface
                            // type resolves to an interface or is an abstract class, new throws
                            // an InstantiationError.
                            if (resolvedClass.isAbstract) {
                                throw InstantiationError("Cannot instantiate abstract class ${inst.desc}")
                            }

                            cWriter.addDependency(resolvedClass)
                            cWriter.write(
                                """
                    |    // new ${inst.desc}
                    |    bc_new(&${resolvedClass.cName});
                    |""".trimMargin()
                            )
                        }
                        Opcodes.ANEWARRAY -> {
                            val componentType = Type.getObjectType(inst.desc)
                            val arrayDesc = when (componentType.sort) {
                                Type.OBJECT -> "[L${inst.desc};"
                                Type.ARRAY -> "[${inst.desc}"
                                else -> throw IllegalArgumentException("Unsupported type ${componentType.sort} for anewarray.")
                            }
                            cWriter.write(
                                """
                    |    // anewarray ${inst.desc}
                    |    bc_newarray("$arrayDesc");
                    |""".trimMargin()
                            )
                        }
                        Opcodes.CHECKCAST -> {
                            val type = Type.getObjectType(inst.desc)
                            when (type.sort) {
                                Type.OBJECT -> {
                                    val resolvedClass = requireNotNull(classPool.getClass(inst.desc)) {
                                        "Could not find class ${inst.desc}"
                                    }.requireClassInfo()

                                    cWriter.addDependency(resolvedClass)
                                    cWriter.write(
                                        """
                    |    // checkcast ${inst.desc}
                    |    bc_checkcast(&${resolvedClass.cName});
                    |""".trimMargin()
                                    )
                                }
                                Type.ARRAY -> {
                                    cWriter.write(
                                        """
                    |    // checkcast ${inst.desc}
                    |    bc_checkcast_a("${inst.desc}");
                    |""".trimMargin()
                                    )
                                }
                                else -> throw IllegalArgumentException("Unsupported type ${type.sort} for instanceof.")
                            }
                        }
                        Opcodes.INSTANCEOF -> {
                            val type = Type.getObjectType(inst.desc)
                            when (type.sort) {
                                Type.OBJECT -> {
                                    val resolvedClass = requireNotNull(classPool.getClass(inst.desc)) {
                                        "Could not find class ${inst.desc}"
                                    }.requireClassInfo()

                                    cWriter.addDependency(resolvedClass)
                                    cWriter.write(
                                        """
                    |    // instanceof ${inst.desc}
                    |    bc_instanceof(&${resolvedClass.cName});
                    |""".trimMargin()
                                    )
                                }
                                Type.ARRAY -> {
                                    cWriter.write(
                                        """
                    |    // instanceof ${inst.desc}
                    |    bc_instanceof_a("${inst.desc}");
                    |""".trimMargin()
                                    )
                                }
                                else -> throw IllegalArgumentException("Unsupported type ${type.sort} for instanceof.")
                            }
                        }
                        else -> throw IllegalArgumentException("Unexpected opcode ${inst.opcode}")
                    }
                }
            }
        }

        cWriter.write(
            """
                    |
                    |    assert(!"Unexpected reach of the end of the method");
                    |    exit(-2);
                    |}
                    |
                    |""".trimMargin()
        )
    }

    private fun writeSwitch(
        cWriter: CWriter,
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

    private fun writeFastNativeMethodBridge(cWriter: CWriter, clazzInfo: ClassInfo, method: MethodInfo) {
        val clazz = clazzInfo.thisClass

        // Write the declaration of the target method
        cWriter.write(
            """
                    |// Fast native method bridge for ${clazz.className}.${method.name}${method.descriptor}
                    |${method.cFunctionDeclarationFastNative};
                    |""".trimMargin()
        )

        if (method.descriptor.returnType.sort == Type.VOID) {
            cWriter.write(
                """
                    |${method.cFunctionDeclaration} {
                    |    ${method.cFunctionNameFastNative}(vmCurrentContext, &${method.cName});
                    |}
                    |
                    |""".trimMargin()
            )
        } else {
            cWriter.write(
                """
                    |${method.cFunctionDeclaration} {
                    |    return ${method.cFunctionNameFastNative}(vmCurrentContext, &${method.cName});
                    |}
                    |
                    |""".trimMargin()
            )
        }
    }

    private fun writeNativeMethodBridge(cWriter: CWriter, clazzInfo: ClassInfo, method: MethodInfo) {
        val clazz = clazzInfo.thisClass
        val jniMethod = JNIMethod(method)

        cWriter.write(
            """
                    |// Native method bridge for ${clazz.className}.${method.name}${method.descriptor}
                    |${method.cFunctionDeclaration} {
                    |    stack_frame_start(&${method.cName}.method, 1 /* for storing return value */, ${jniMethod.localSlotCount});
                    |""".trimMargin()
        )

        // Pass arguments
        if (jniMethod.argumentCount > 0) {
            cWriter.write(
                """
                    |    bc_prepare_arguments(${jniMethod.argumentCount});
                    |""".trimMargin()
            )
        }
        if(!jniMethod.isStatic) {
            cWriter.write(
                """
                    |    bc_check_objectref();
                    |""".trimMargin()
            )
        }

        if(method.isSynchronized) {
            // Need an empty try for releasing the lock before exception is thrown up
            cWriter.write(
                """
                    |
                    |    // Empty exception frame for freeing the lock of synchronized method
                    |    exception_frame_start();
                    |        exception_not_handled();
                    |    exception_block_end();
                    |""".trimMargin()
            )
        }

        // Resolve the function pointer to the native method
        cWriter.write(
            """
                |
                |    // Resolve the function ptr
                |    ${jniMethod.functionPtrDecl("ptr")};
                |    ptr = bc_resolve_native(vmCurrentContext, &${method.cName});
                |
                |""".trimMargin()
        )

        // Prepare native stack frame
        cWriter.write(
            """
                |    // Prepare native frame
                |    bc_native_prepare();
                |""".trimMargin()
        )

        // Prepare handler for argument that is object reference
        cWriter.write(
                """
                |
                |    // Prepare arguments
                |${jniMethod.referenceArgumentsPreparationCode}
                |""".trimMargin()
        )

        // Call real function
        val hasReturnValue = jniMethod.returnType.sort != Type.VOID
        if (hasReturnValue) {
            cWriter.write(
                """
                |    // Call real function
                |    bc_native_start();
                |    ${jniMethod.returnType.toJNIType()} result = ptr(
                |""".trimMargin()
            )
        } else {
            cWriter.write(
                """
                |    // Call real function
                |    bc_native_start();
                |    ptr(
                |""".trimMargin()
            )
        }

        cWriter.write(
            """
                |        ${jniMethod.argumentsPassingCode}
                |    );
                |""".trimMargin()
        )

        // Handle return
        val returnPrefix = returnTypePrefixes.getValue(method.descriptor.returnType.sort)
        if(hasReturnValue) {
            cWriter.write(
                """
                    |    bc_native_end_${returnPrefix}(result);
                    |""".trimMargin()
            )
        } else {
            cWriter.write(
                """
                    |    bc_native_end_o(JAVA_NULL);
                    |""".trimMargin()
            )
        }

        cWriter.write(
            """
                    |
                    |    bc_${returnPrefix}return();
                    |}
                    |
                    |""".trimMargin()
        )
    }

    /** jvms8 §6.5 Instructions - invokespecial */
    private fun writeInvokeSpecial(cWriter: CWriter, inst: MethodInsnNode, currentClass: ClassInfo, ownerClass: ClassInfo, resolvedMethod: MethodInfo) {
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
            ).filter { !it.isAbstract }
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
        cWriter.addDependency(methodToInvoke.declaringClass)
        cWriter.write(
            """
                    |    // invokespecial ${inst.owner}.${inst.name}${inst.desc}
                    |    bc_invoke_special${methodToInvoke.invokeSuffix}(${methodToInvoke.cFunctionName});
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

            Opcodes.LCMP  to "bc_lcmp",
            Opcodes.FCMPL to "bc_fcmpl",
            Opcodes.FCMPG to "bc_fcmpg",
            Opcodes.DCMPL to "bc_dcmpl",
            Opcodes.DCMPG to "bc_dcmpg",

            Opcodes.ARRAYLENGTH to "bc_arraylength",

            Opcodes.CALOAD to "bc_caload",
            Opcodes.BALOAD to "bc_baload",
            Opcodes.SALOAD to "bc_saload",
            Opcodes.IALOAD to "bc_iaload",
            Opcodes.FALOAD to "bc_faload",
            Opcodes.LALOAD to "bc_laload",
            Opcodes.DALOAD to "bc_daload",
            Opcodes.AALOAD to "bc_aaload",

            Opcodes.CASTORE to "bc_castore",
            Opcodes.BASTORE to "bc_bastore",
            Opcodes.SASTORE to "bc_sastore",
            Opcodes.IASTORE to "bc_iastore",
            Opcodes.FASTORE to "bc_fastore",
            Opcodes.LASTORE to "bc_lastore",
            Opcodes.DASTORE to "bc_dastore",
            Opcodes.AASTORE to "bc_aastore",

            Opcodes.MONITORENTER to "bc_monitorenter",
            Opcodes.MONITOREXIT  to "bc_monitorexit",

            Opcodes.ATHROW to "bc_athrow"
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

        private val byteCodesNewArrayType = mapOf(
            Opcodes.T_BOOLEAN   to "[Z",
            Opcodes.T_CHAR      to "[C",
            Opcodes.T_FLOAT     to "[F",
            Opcodes.T_DOUBLE    to "[D",
            Opcodes.T_BYTE      to "[B",
            Opcodes.T_SHORT     to "[S",
            Opcodes.T_INT       to "[I",
            Opcodes.T_LONG      to "[J"
        )

        // jvms8 §4.9.2 Structural Constraints
        private val byteCodesReturn = mapOf(
            // – All instance initialization methods, class or interface initialization methods,
            //   and methods declared to return void must use only the return instruction.
            Opcodes.RETURN to Pair("return", mapOf(Type.VOID to "")),
            // – If the method returns a boolean, byte, char, short, or int, only the ireturn
            //   instruction may be used.
            Opcodes.IRETURN to Pair(
                "ireturn", mapOf(
                    Type.BOOLEAN to "z",
                    Type.CHAR to "c",
                    Type.BYTE to "b",
                    Type.SHORT to "s",
                    Type.INT to "i"
                )
            ),
            // – If the method returns a float, long, or double, only an freturn, lreturn, or
            //   dreturn instruction, respectively, may be used.
            Opcodes.FRETURN to Pair("freturn", mapOf(Type.FLOAT to "f")),
            Opcodes.LRETURN to Pair("lreturn", mapOf(Type.LONG to "l")),
            Opcodes.DRETURN to Pair("dreturn", mapOf(Type.DOUBLE to "d")),
            // – If the method returns a reference type, only an areturn instruction may be
            //   used, and the type of the returned value must be assignment compatible with
            //   the return descriptor of the method (JLS §5.2, §4.3.3).
            Opcodes.ARETURN to Pair(
                "areturn", mapOf(
                    Type.ARRAY to "a",
                    Type.OBJECT to "o"
                )
            )
        )

        private val returnTypePrefixes: Map<Int, String> = byteCodesReturn
                .map { it.value.second }
                .reduce { acc, map -> acc + map }
    }
}
