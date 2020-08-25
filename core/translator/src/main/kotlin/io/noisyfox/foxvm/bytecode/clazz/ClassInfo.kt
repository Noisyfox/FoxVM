package io.noisyfox.foxvm.bytecode.clazz

import org.objectweb.asm.Opcodes

data class ClassInfo(
    val thisClass: Clazz,
    val cIdentifier: String,
    val version: Int,
    val signature: String?,
    val superClass: Clazz?,
    val interfaces: List<Clazz>,
    val fields: MutableList<FieldInfo> = mutableListOf(),
    val methods: MutableList<MethodInfo> = mutableListOf(),
    val preResolvedStaticFields: MutableList<PreResolvedStaticFieldInfo> = mutableListOf(),
    val preResolvedInstanceFields: MutableList<PreResolvedInstanceFieldInfo> = mutableListOf()
) {

    val isEnum: Boolean = (thisClass.access and Opcodes.ACC_ENUM) == Opcodes.ACC_ENUM

    val isInterface: Boolean = (thisClass.access and Opcodes.ACC_INTERFACE) == Opcodes.ACC_INTERFACE

    /** The finalizer declared by this class */
    val declFinalizer: MethodInfo?
        get() = when {
            isEnum -> {
                // Enums do not have finalizer
                null
            }
            thisClass.className in UnfinalizableClasses -> {
                null
            }
            else -> {
                methods.firstOrNull { it.isFinalizer }
            }
        }

    /** The finalizer declared by this class or inherited from [superClass] */
    val finalizer: MethodInfo?
        get() = declFinalizer ?: superClass?.requireClassInfo()?.finalizer

    infix fun instanceOf(superClass: ClassInfo): Boolean {
        if (this == superClass) {
            return true
        }

        return thisClass.allSuperClasses().any { it.requireClassInfo() == this }
    }

    /**
     * Look up the field referenced by [inst] in current class.
     *
     * See jvms8 §5.4.3.2 Field Resolution for more details
     *
     * @return the [FieldInfo] if the referenced field is found, `null` otherwise.
     */
    fun fieldLookup(name: String, desc: String): FieldInfo? {
        // 1. If C declares a field with the name and descriptor specified by the field
        //    reference, field lookup succeeds. The declared field is the result of the field
        //    lookup.
        val decls = this.fields.filter { it.matches(name, desc) }
        when (decls.size) {
            0 -> {
            }
            1 -> return decls.single()
            else -> throw ClassFormatError("Multiple fields with same name and descriptor found in class ${this.thisClass.className}: $decls")
        }

        // 2. Otherwise, field lookup is applied recursively to the direct superinterfaces of
        //    the specified class or interface C.
        this.interfaces.forEach {
            val f = it.requireClassInfo().fieldLookup(name, desc)
            if (f != null) {
                return f
            }
        }

        // 3. Otherwise, if C has a superclass S, field lookup is applied recursively to S.
        val f = this.superClass?.requireClassInfo()?.fieldLookup(name, desc)
        if (f != null) {
            return f
        }

        // 4. Otherwise, field lookup fails.
        return null
    }

    /**
     * Lookup a method symbolic reference which the owner class is `this`.
     *
     * See jvms8 §5.4.3.3 Method Resolution and §5.4.3.4 Interface Method Resolution for more details
     */
    fun methodLookup(name: String, desc: String): MethodInfo? {
        if (isInterface) {
            // Use interface method resolution

            // 2. ..., if C declares a method with the name and descriptor specified by
            //    the interface method reference, method lookup succeeds.
            val decl = findDeclMethod(name, desc)
            if (decl != null) {
                return decl
            }

            // 3. Otherwise, if the class Object declares a method with the name and descriptor
            //    specified by the interface method reference, which has its ACC_PUBLIC flag set
            //    and does not have its ACC_STATIC flag set, method lookup succeeds.
            // Find the Object class
            val objectClass = requireNotNull(this.superClass) {
                "Interface ${this.thisClass.className} does not have a super class"
            }.requireClassInfo()
            require(objectClass.thisClass.className == Clazz.CLASS_JAVA_LANG_OBJECT) {
                "The super class of a interface can only be ${Clazz.CLASS_JAVA_LANG_OBJECT}, but class ${this.thisClass.className} has ${objectClass.thisClass.className} instead"
            }
            val objDecl = objectClass.findDeclMethod(name, desc)
                ?.takeIf { it.isPublic && !it.isStatic }
            if (objDecl != null) {
                return objDecl
            }
        } else {
            // Use normal method resolution

            // 2. ..., method resolution attempts to locate the referenced method in C
            //    and its superclasses
            val decl = methodResolutionInClass(name, desc)
            if (decl != null) {
                return decl
            }
        }

        // Otherwise, method resolution attempts to locate the referenced method in the
        // superinterfaces of the specified class C:
        val allSuperInterfaces = thisClass.allSuperClasses().filter { it.requireClassInfo().isInterface }
        // If the maximally-specific superinterface methods of C for the name and
        // descriptor specified by the method reference include exactly one method that
        // does not have its ACC_ABSTRACT flag set, then this method is chosen and
        // method lookup succeeds.
        val maxSpecMethods = findMaximallySpecificSuperInterfaceMethod(allSuperInterfaces, name, desc)
            .filter { !it.isAbstract }
        when (maxSpecMethods.size) {
            1 -> return maxSpecMethods.single()
            else -> {
                // TODO: add debug output
            }
        }

        // Otherwise, if any superinterface of C declares a method with the name and
        // descriptor specified by the method reference that has neither its ACC_PRIVATE
        // flag nor its ACC_STATIC flag set, one of these is arbitrarily chosen and method
        // lookup succeeds.
        val superInterfaceDecls = allSuperInterfaces.mapNotNull { superInterface ->
            superInterface.requireClassInfo()
                // a method with the name and descriptor specified by the method reference
                .findDeclMethod(name, desc)
                // that has neither its ACC_PRIVATE flag nor its ACC_STATIC flag set
                ?.takeIf { method -> !method.isPrivate && !method.isStatic }
        }
        when (superInterfaceDecls.size) {
            0 -> {
            }
            else -> return superInterfaceDecls.first()
        }

        // Otherwise, method lookup fails.
        return null
    }

    /**
     * A maximally-specific superinterface method of a class or interface C for a particular
     * method name and descriptor is any method for which all of the following are true:
     * • The method is declared in a superinterface (direct or indirect) of C.
     * • The method is declared with the specified name and descriptor.
     * • The method has neither its ACC_PRIVATE flag nor its ACC_STATIC flag set.
     * • Where the method is declared in interface I, there exists no other maximallyspecific
     *   superinterface method of C with the specified name and descriptor that
     *   is declared in a subinterface of I.
     */
    private fun findMaximallySpecificSuperInterfaceMethod(allSuperInterfaces: List<Clazz>, name: String, desc: String): List<MethodInfo> {
        // First we find all methods that matches the first 3 rules
        val allMethods =
            allSuperInterfaces.mapNotNull { superInterface -> // The method is declared in a superinterface (direct or indirect) of C.
                superInterface.requireClassInfo()
                    // The method is declared with the specified name and descriptor.
                    .findDeclMethod(name, desc)
                    // The method has neither its ACC_PRIVATE flag nor its ACC_STATIC flag set.
                    ?.takeIf { method -> !method.isPrivate && !method.isStatic }
            }

        // Then we find methods that match rule 4
        return allMethods.filter { cand ->
            // Where the method is declared in interface I, there exists no other maximallyspecific
            // superinterface method of C with the specified name and descriptor that
            // is declared in a subinterface of I.
            val I = cand.declaringClass
            allMethods.none { other ->
                other.declaringClass != I && (other.declaringClass instanceOf I)
            }
        }
    }

    /**
     * The step 2 of jvms8 §5.4.3.3 Method Resolution
     *
     * > Otherwise, method resolution attempts to locate the referenced method in C
     * > and its superclasses
     */
    private fun methodResolutionInClass(name: String, desc: String): MethodInfo? {
        // If C declares exactly one method with the name specified by the method
        // reference, and the declaration is a signature polymorphic method (§2.9), then
        // method lookup succeeds. All the class names mentioned in the descriptor
        // are resolved (§5.4.3.1).
        // TODO: we don't support signature polymorphic method yet

        // Otherwise, if C declares a method with the name and descriptor specified by
        // the method reference, method lookup succeeds.
        val decl = findDeclMethod(name, desc)
        if (decl != null) {
            return decl
        }

        // Otherwise, if C has a superclass, step 2 of method resolution is recursively
        // invoked on the direct superclass of C.
        return this.superClass?.requireClassInfo()?.methodResolutionInClass(name, desc)
    }

    private fun findDeclMethod(name: String, desc: String): MethodInfo?{
        val decls = methods.filter { it.matches(name, desc) }
        return when (decls.size) {
            0 -> null
            1 -> decls.single()
            else -> throw ClassFormatError("Multiple methods with same name and descriptor found in class ${this.thisClass.className}: $decls")
        }
    }

    override fun toString(): String {
        return "${ClassInfo::class.simpleName} [${thisClass.className}]"
    }

    private companion object {
        private val UnfinalizableClasses = setOf(
            Clazz.CLASS_JAVA_LANG_OBJECT,
            Clazz.CLASS_JAVA_LANG_ENUM
        )
    }
}
