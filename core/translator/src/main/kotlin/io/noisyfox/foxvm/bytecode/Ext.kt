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
 * Convert the given string to a C string that is encoded in UTF-8.
 */
fun String.asCString(): String {
    val sb = StringBuilder(length)

    var lastIsHexEscape = false
    this.forEach { c ->
        when (c) {
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

                val bytes = c.toString().toByteArray()
                sb.append("\\x" + bytes.joinToString("\\x") { "%02x".format(it) })
            }
        }
    }

    return sb.toString().intern()
}
