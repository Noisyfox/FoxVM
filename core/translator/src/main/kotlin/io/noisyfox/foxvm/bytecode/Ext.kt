package io.noisyfox.foxvm.bytecode

import io.noisyfox.foxvm.bytecode.clazz.MethodInfo
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
fun String.asCIdentifier(): String = asSequence().joinToString(separator = "") { it.asCIdentifier() }.intern()

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

// https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/design.html
// Resolving Native Method Names
// Dynamic linkers resolve entries based on their names. A native method name is concatenated
// from the following components:
// • the prefix Java_
// • a mangled fully-qualified class name
// • an underscore (“_”) separator
// • a mangled method name
// • for overloaded native methods, two underscores (“__”) followed by the mangled argument signature
// The VM checks for a method name match for methods that reside in the native library. The VM looks
// first for the short name; that is, the name without the argument signature. It then looks for the
// long name, which is the name with the argument signature.
val MethodInfo.jniShortName: String
    get() = "Java_${this.declaringClass.thisClass.className.asJNIIdentifier()}_${this.name.asJNIIdentifier()}"

val MethodInfo.jniLongName: String
    get() {
        val desc = this.descriptor.descriptor
        // Keep the argument desc only
        val argDesc = desc.substring(1, desc.lastIndexOf(')'))
        return "${this.jniShortName}__${argDesc.asJNIIdentifier()}"
    }


// We adopted a simple name-mangling scheme to ensure that all Unicode characters translate into valid
// C function names. We use the underscore (“_”) character as the substitute for the slash (“/”) in fully
// qualified class names. Since a name or type descriptor never begins with a number, we can use
// _0, ..., _9 for escape sequences, as the following table illustrates:
//
// Unicode Character Translation
// +-----------------+----------------------
// | Escape Sequence | Denotes
// +-----------------+----------------------
// | _0XXXX          | a Unicode character XXXX. Note that lower case is used to represent non-ASCII Unicode
// |                 | characters, for example, _0abcd as opposed to _0ABCD.
// +-----------------+----------------------
// | _1              | the character “_”
// +-----------------+----------------------
// | _2              | the character “;” in signatures
// +-----------------+----------------------
// | _3              | the character “[“ in signatures
// +-----------------+----------------------
private fun Char.asJNIIdentifier(): String = when (this) {
    in '0'..'9', in 'a'..'z', in 'A'..'Z' -> this.toString()
    '/' -> "_"
    '_' -> "_1"
    ';' -> "_2"
    '[' -> "_3"
    else -> "_0%04x".format(this.toInt())
}

private fun String.asJNIIdentifier(): String = asSequence().joinToString(separator = "") { it.asJNIIdentifier() }.intern()
