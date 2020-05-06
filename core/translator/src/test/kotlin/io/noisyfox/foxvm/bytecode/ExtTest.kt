package io.noisyfox.foxvm.bytecode

import kotlin.test.Test
import kotlin.test.assertEquals

class ExtTest {

    @Test
    fun `test String#asCString()`() {
        assertEquals("""abc""\xe6\xb5\x8b\xe8\xaf\x95\xe4\xb8\x80\xe4\xb8\x8b""abc\r\n\t\\\"""", "abc测试一下abc\r\n\t\\\"".asCString())
    }

}