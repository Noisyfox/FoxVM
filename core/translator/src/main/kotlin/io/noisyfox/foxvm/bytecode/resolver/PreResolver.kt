package io.noisyfox.foxvm.bytecode.resolver

import io.noisyfox.foxvm.bytecode.ClassPool
import io.noisyfox.foxvm.bytecode.asCIdentifier
import io.noisyfox.foxvm.bytecode.clazz.ClassInfo
import io.noisyfox.foxvm.bytecode.clazz.Clazz
import io.noisyfox.foxvm.bytecode.clazz.FieldInfo
import io.noisyfox.foxvm.bytecode.clazz.MethodInfo
import io.noisyfox.foxvm.bytecode.clazz.PreResolvedInstanceFieldInfo
import io.noisyfox.foxvm.bytecode.clazz.PreResolvedStaticFieldInfo
import io.noisyfox.foxvm.bytecode.visitor.ClassHandler
import org.objectweb.asm.ClassReader
import org.objectweb.asm.ClassVisitor
import org.objectweb.asm.FieldVisitor
import org.objectweb.asm.MethodVisitor
import org.objectweb.asm.Opcodes
import org.objectweb.asm.Type
import org.objectweb.asm.commons.JSRInlinerAdapter
import org.objectweb.asm.tree.MethodNode
import org.slf4j.LoggerFactory

class PreResolver(
    private val classPool: ClassPool
) : ClassHandler {

    var resolvedClasses: Long = 0
        private set

    override fun handleAnyClass(clazz: Clazz) {
        if (clazz.classInfo != null) {
            return
        }

        LOGGER.debug("Pre-resolving class [{}].", clazz.className)
        resolvedClasses++

        val options = if (clazz.isRuntimeClass) {
            // Don't parse code for runtime classes
            ClassReader.SKIP_FRAMES or ClassReader.SKIP_CODE
        } else {
            // For now we don't generate codes for frame verification, will consider adding this later
            ClassReader.SKIP_FRAMES
        }

        clazz.accept(PreResolverClassVisitor(classPool, this, clazz), options)
    }

    companion object {
        private val LOGGER = LoggerFactory.getLogger(PreResolver::class.java)!!
    }
}

private class PreResolverClassVisitor(
    private val classPool: ClassPool,
    private val resolver: PreResolver,
    private val clazz: Clazz
) : ClassVisitor(Opcodes.ASM8) {

    private fun ensureResolved(className: String): Clazz {
        val clazz = requireNotNull(classPool.getClass(className), {
            LOGGER.error(
                "Unable to pre-resolve class [{}]: unable to find dependency class [{}].",
                clazz.className,
                className
            )
            "Class not found: [${className}]"
        })

        clazz.accept(resolver)

        return clazz
    }

    override fun visit(
        version: Int,
        access: Int,
        name: String,
        signature: String?,
        superName: String?,
        interfaces: Array<String>?
    ) {
        assert(clazz.classInfo == null)

        /**
         * jvms8 $5.3.5 Deriving a Class from a class File Representation
         */
        val info = ClassInfo(
            thisClass = clazz,
            cIdentifier = mangleClassName(clazz.className),
            version = version,
            signature = signature,
            // 3. If C has a direct superclass, the symbolic reference from C to its direct superclass
            //    is resolved using the algorithm of §5.4.3.1.
            superClass = superName?.let(this::ensureResolved),
            // 4. If C has any direct superinterfaces, the symbolic references from C to its direct
            //    superinterfaces are resolved using the algorithm of §5.4.3.1.
            interfaces = interfaces?.map(this::ensureResolved)?.toList() ?: emptyList()
        )

        // Note that if C is an interface it must
        // have Object as its direct superclass, which must already have been loaded.
        if (info.isInterface) {
            requireNotNull(info.superClass).let {
                if (it.className != Clazz.CLASS_JAVA_LANG_OBJECT) {
                    throw ClassFormatError("interface $clazz does not have ${Clazz.CLASS_JAVA_LANG_OBJECT} as superclass")
                }
            }
        }
        // Only Object has no direct superclass.
        if (clazz.className == Clazz.CLASS_JAVA_LANG_OBJECT) {
            if (info.superClass != null) {
                throw ClassFormatError("class $clazz cannot have superclass")
            }
        } else {
            if (info.superClass == null) {
                throw ClassFormatError("class $clazz must have a superclass")
            }
        }

        info.superClass?.let { s ->
            // Continue the superclass resolution
            // 5.4.3.1 Class and Interface Resolution
            // 3. Finally, access permissions to C are checked.
            //    • If C is not accessible (§5.4.4) to D, class or interface resolution throws an
            //      IllegalAccessError.
            if (!info.canAccess(s.requireClassInfo())) {
                throw IllegalAccessError("tried to access class $s from class $info")
            }

            // • If the class or interface named as the direct superclass of C is in fact an
            //   interface, loading throws an IncompatibleClassChangeError.
            if (s.requireClassInfo().isInterface) {
                throw IncompatibleClassChangeError("superclass of $info: $s is an interface")
            }

            // Merge all instance fields from super class
            info.preResolvedInstanceFields.addAll(s.requireClassInfo().preResolvedInstanceFields)
        }

        info.interfaces.forEach { i ->
            // Continue the superinterface resolution
            // 5.4.3.1 Class and Interface Resolution
            // 3. Finally, access permissions to C are checked.
            //    • If C is not accessible (§5.4.4) to D, class or interface resolution throws an
            //      IllegalAccessError.
            if (!info.canAccess(i.requireClassInfo())) {
                throw IllegalAccessError("tried to access interface $i from class $info")
            }

            // • If any of the classes or interfaces named as direct superinterfaces of C is not
            //   in fact an interface, loading throws an IncompatibleClassChangeError.
            if (!i.requireClassInfo().isInterface) {
                throw IncompatibleClassChangeError("superinterfaces of $info: $i is not an interface")
            }
        }

        // • Otherwise, if any of the superclasses/superinterfaces of C is C itself, loading throws a
        //   ClassCircularityError.
        if (clazz in info.allSuperClasses()) {
            throw ClassCircularityError("class $clazz has it's own as superclasses/superinterfaces")
        }

        clazz.classInfo = info
    }

    override fun visitField(
        access: Int,
        name: String,
        descriptor: String,
        signature: String?,
        value: Any?
    ): FieldVisitor? {
        val info = clazz.requireClassInfo()

        val field = FieldInfo(
            declaringClass = info,
            access = access,
            name = name.intern(),
            cIdentifier = mangleFieldName(name),
            descriptor = Type.getType(descriptor.intern()),
            signature = signature,
            defaultValue = value
        )

        info.fields.add(field)

        return null
    }

    override fun visitMethod(
        access: Int,
        name: String,
        descriptor: String,
        signature: String?,
        exceptions: Array<out String>?
    ): MethodVisitor {
        val info = clazz.requireClassInfo()

        // Signature and exceptions are ignored atm
        val method = MethodInfo(
            declaringClass = info,
            access = access,
            name = name.intern(),
            cIdentifier = mangleMethodName(name),
            descriptor = Type.getMethodType(descriptor.intern()),
            signature = signature,
            methodNode = MethodNode(Opcodes.ASM8, access, name, descriptor, signature, exceptions)
        )

        info.methods.add(method)

        return JSRInlinerAdapter(method.methodNode, access, name, descriptor, signature, exceptions)
    }

    override fun visitEnd() {
        // Do pre-resolving
        val info = clazz.requireClassInfo()

        val instanceFields = mutableListOf<PreResolvedInstanceFieldInfo>()

        info.fields.forEachIndexed { i, f ->
            if (f.isStatic) {
                info.preResolvedStaticFields.add(
                    PreResolvedStaticFieldInfo(
                        field = f,
                        fieldIndex = i,
                        isReference = f.isReference
                    )
                )
            } else {
                instanceFields.add(
                    PreResolvedInstanceFieldInfo(
                        field = f,
                        fieldIndex = i,
                        isReference = f.isReference
                    )
                )
            }
        }

        // Sort fields based on types
        info.preResolvedStaticFields.sortWith(compareBy(FieldLayout) { info.fields[it.fieldIndex].descriptor })
        instanceFields.sortWith(compareBy(FieldLayout) { info.fields[it.fieldIndex].descriptor })

        info.preResolvedInstanceFields.addAll(instanceFields)
    }

    companion object {
        private val LOGGER = LoggerFactory.getLogger(PreResolverClassVisitor::class.java)!!
    }
}

/**
 * <package_name>/<class_name> ->
 * <package_identifier_len>P<package_identifier><class_identifier_len>C<class_identifier>
 *
 * aaa/bbbb/ccccc/DDD$EE$FFF -> 14Paaa_bbbb_ccccc10CDDD_EE_FFF
 * for class name does not have a package:
 * DDD$EE$FFF -> 0P10CDDD_EE_FFF
 */
fun mangleClassName(className: String): String {
    val comp = className.split('/')

    val packageIdentifier = comp.subList(0, comp.size - 1).joinToString("/").asCIdentifier()

    val classIdentifier = comp.last().asCIdentifier()

    return "${packageIdentifier.length}P$packageIdentifier${classIdentifier.length}C$classIdentifier".intern()
}

/**
 * <field_name> -> <field_identifier_len>F<field_identifier>
 */
fun mangleFieldName(fieldName: String): String {
    val fieldIdentifier = fieldName.asCIdentifier()
    return "${fieldIdentifier.length}F$fieldIdentifier".intern()
}

/**
 * <init> -> 4IINIT     // For initializer of instance
 * <clinit> -> 6ICLINIT // For initializer of class
 * <method_name> -> <method_identifier_len>M<method_identifier>
 */
fun mangleMethodName(methodName: String): String {
    return when (methodName) {
        MethodInfo.INIT -> "4IINIT"
        MethodInfo.CLINIT -> "6ICLINIT"
        else -> {
            val methodIdentifier = methodName.asCIdentifier()
            return "${methodIdentifier.length}M$methodIdentifier".intern()
        }
    }
}
