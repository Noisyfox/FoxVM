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
