package io.noisyfox.foxvm.bytecode.resolver

import io.noisyfox.foxvm.bytecode.ClassPool
import io.noisyfox.foxvm.bytecode.clazz.ClassInfo
import io.noisyfox.foxvm.bytecode.clazz.Clazz
import io.noisyfox.foxvm.bytecode.clazz.FieldInfo
import io.noisyfox.foxvm.bytecode.clazz.PreResolvedInstanceFieldInfo
import io.noisyfox.foxvm.bytecode.clazz.PreResolvedStaticFieldInfo
import io.noisyfox.foxvm.bytecode.visitor.ClassHandler
import org.objectweb.asm.ClassReader
import org.objectweb.asm.ClassVisitor
import org.objectweb.asm.FieldVisitor
import org.objectweb.asm.Opcodes
import org.objectweb.asm.Type
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
        clazz.accept(PreResolverClassVisitor(classPool, this, clazz), ClassReader.SKIP_CODE)
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

        val info = ClassInfo(
            version = version,
            superClass = superName?.let(this::ensureResolved),
            interfaces = interfaces?.map(this::ensureResolved)?.toList() ?: emptyList()
        )

        info.superClass?.let { s ->
            // Merge all instance fields from super class
            info.preResolvedInstanceFields.addAll(s.classInfo!!.preResolvedInstanceFields)
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
        val info = requireNotNull(clazz.classInfo)

        val field = FieldInfo(
            access = access,
            name = name.intern(),
            descriptor = Type.getType(descriptor.intern()),
            defaultValue = value
        )

        info.fields.add(field)

        return null
    }

    override fun visitEnd() {
        // Do pre-resolving
        val info = requireNotNull(clazz.classInfo)

        val instanceFields = mutableListOf<PreResolvedInstanceFieldInfo>()

        info.fields.forEachIndexed { i, f ->
            if (f.isStatic) {
                info.preResolvedStaticFields.add(
                    PreResolvedStaticFieldInfo(
                        fieldIndex = i,
                        isReference = f.isReference
                    )
                )
            } else {
                instanceFields.add(
                    PreResolvedInstanceFieldInfo(
                        declaringClass = clazz,
                        fieldIndex = i,
                        isReference = f.isReference
                    )
                )
            }
        }

        // Sort fields based on types
        info.preResolvedStaticFields.sortWith(compareBy(FieldLayout, { info.fields[it.fieldIndex].descriptor }))
        instanceFields.sortWith(compareBy(FieldLayout, { info.fields[it.fieldIndex].descriptor }))

        info.preResolvedInstanceFields.addAll(instanceFields)
    }

    companion object {
        private val LOGGER = LoggerFactory.getLogger(PreResolverClassVisitor::class.java)!!
    }
}