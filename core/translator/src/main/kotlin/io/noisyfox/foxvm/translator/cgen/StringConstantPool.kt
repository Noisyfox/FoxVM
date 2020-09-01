package io.noisyfox.foxvm.translator.cgen

class StringConstantPool : Iterable<String> {
    private val pool: LinkedHashSet<String> = LinkedHashSet()

    fun addConstant(constant: String): Int {
        pool.add(constant)

        return pool.size - 1
    }

    override fun iterator(): Iterator<String> = pool.iterator()

    val count: Int
        get() = pool.size

}
