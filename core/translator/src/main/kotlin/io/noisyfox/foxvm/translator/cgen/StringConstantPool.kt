package io.noisyfox.foxvm.translator.cgen

class StringConstantPool : Iterable<String> {
    private val pool: LinkedHashMap<String, Int> = LinkedHashMap()

    init {
        // Pre-defined constants, must be sync with native/runtime/include/vm_string.h
        pool["<init>"] = 0
        pool["<clinit>"] = 1
        pool["clone"] = 2
        pool["()Ljava/lang/Object;"] = 3
    }

    fun addConstant(constant: String): Int {
        return pool.getOrPut(constant) {
            pool.size
        }
    }

    override fun iterator(): Iterator<String> = pool.keys.iterator()

    val count: Int
        get() = pool.size

}
