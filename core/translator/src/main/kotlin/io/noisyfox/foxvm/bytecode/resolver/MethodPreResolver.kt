package io.noisyfox.foxvm.bytecode.resolver

import io.noisyfox.foxvm.bytecode.ClassPool
import io.noisyfox.foxvm.bytecode.clazz.Clazz
import io.noisyfox.foxvm.bytecode.clazz.MethodInfo
import org.objectweb.asm.MethodVisitor
import org.objectweb.asm.Opcodes

class MethodPreResolver(
    private val method: MethodInfo,
    private val classPool: ClassPool,
    private val resolver: PreResolver,
    private val clazz: Clazz
): MethodVisitor(Opcodes.ASM8) {

}