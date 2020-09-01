package io.noisyfox.foxvm.bytecode

import org.objectweb.asm.Type

fun Type.isReference() = when (this.sort) {
    Type.OBJECT, Type.ARRAY -> true
    else -> false
}

/**
 * Allowed range: https://source.android.com/devices/tech/dalvik/dex-format#simplename
 * except the Unicode supplemental characters (U+10000 … U+10ffff)
 */
private fun Char.asCIdentifier(): String = when (this) {
    // Additionally support "/"
    '/' -> "_"

    ' ', '$', '-', '_' -> "_"

    in 'A'..'Z',
    in 'a'..'z',
    in '0'..'9' -> this.toString()

    in '\u00a0'..'\u200a',
    in '\u2010'..'\u2027',
    in '\u202f'..'\ud7ff',
    in '\ue000'..'\uffef' -> "u%04x".format(this.toInt())

    else -> throw IllegalArgumentException("Unsupported character \"0x${"%04x".format(this.toInt())}\" for JVM identifier.")
}

/**
 * Convert the given string to a C compatible identifier
 */
fun String.asCIdentifier(): String {
    val sb = StringBuilder(length)

    this.forEach {
        sb.append(it.asCIdentifier())
    }

    return sb.toString().intern()
}

/**
 * Convert the given string to a C string that is encoded in modified UTF-8 (§4.4.7).
 */
fun String.asCString(): String {
    val sb = StringBuilder(length)

    var lastIsHexEscape = false
    this.forEach { c ->
        when (c) {
            // • Code points in the range '\u0001' to '\u007F' are represented by a single byte:
            //   0 bits 6-0
            //   The 7 bits of data in the byte give the value of the code point represented.

            // Common escapes
            '\b' -> sb.append("\\b")
            '\n' -> sb.append("\\n")
            '\r' -> sb.append("\\r")
            '\t' -> sb.append("\\t")
            '\\' -> sb.append("\\\\")
            '\"' -> sb.append("\\\"")

            // Printable ascii characters
            in 0x20.toChar()..0x7F.toChar() -> {
                if (lastIsHexEscape) {
                    lastIsHexEscape = false
                    // Add two double quote to force terminating an hex escape
                    sb.append("\"\"")
                }

                sb.append(c)
            }
            // Encode all other characters using "\x00" sequence
            else -> {
                if (!lastIsHexEscape) {
                    // Start escaping with double quote
                    sb.append("\"\"")
                }
                lastIsHexEscape = true

                val charValue = c.toInt()
                val bytes = if (charValue >= 0x0001 && charValue <= 0x007F) {
                    byteArrayOf(charValue.toByte())
                } else if (charValue <= 0x07FF) {
                    // • The null code point ('\u0000') and code points in the range '\u0080' to '\u07FF'
                    //   are represented by a pair of bytes x and y :
                    //   x: 1 1 0 bits 10-6
                    //   y: 1 0 bits 5-0
                    byteArrayOf(
                        (0xC0 or ((c.toInt() shr 6) and 0x1F)).toByte(),
                        (0x80 or ((c.toInt() and 0x3F))).toByte()
                    )
                } else {
                    // • Code points in the range '\u0800' to '\uFFFF' are represented by 3 bytes x, y,
                    //   and z :
                    //   x: 1 1 1 0 bits 15-12
                    //   y: 1 0 bits 11-6
                    //   z: 1 0 bits 5-0
                    byteArrayOf(
                        (0xE0 or ((c.toInt() shr 12) and 0xF)).toByte(),
                        (0x80 or ((c.toInt() shr 6) and 0x3F)).toByte(),
                        (0x80 or ((c.toInt() and 0x3F))).toByte()
                    )
                }

                // Make sure there are no `NULL`s in the encoded string
                assert(bytes.none { it == 0.toByte() })

                sb.append("\\x" + bytes.joinToString("\\x") { "%02x".format(it) })
            }
        }
    }

    return sb.toString().intern()
}
