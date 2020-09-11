package io.noisyfox.foxvm.translator.cgen

class StringConstantPool : Iterable<String> {
    private val pool: LinkedHashMap<String, Int> = LinkedHashMap()

    fun addConstant(constant: String): Int {
        return pool.getOrPut(constant) {
            pool.size
        }
    }

    override fun iterator(): Iterator<String> = pool.keys.iterator()

    val count: Int
        get() = pool.size

}
