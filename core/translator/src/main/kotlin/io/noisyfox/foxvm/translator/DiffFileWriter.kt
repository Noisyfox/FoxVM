package io.noisyfox.foxvm.translator

import java.io.File
import java.io.Writer

/**
 * A file writer that only touch the existing file if the content has changed.
 */
class DiffFileWriter(
    private val outFile: File
) : Writer() {
    private var closed: Boolean = false
    private var filerWriter: Writer? = null
    private var oldContent: CharArray?
    private var cursor: Int = 0

    var didUpdateFile: Boolean = false
        private set

    init {
        oldContent = try {
            if (outFile.exists() && outFile.isFile) {
                outFile.readText().toCharArray()
            } else {
                null
            }
        } catch (e: Throwable) {
            e.printStackTrace()
            null
        }

        if (oldContent == null) {
            didUpdateFile = true
            filerWriter = outFile.writer()
        }
    }

    override fun write(cbuf: CharArray, off: Int, len: Int) {
        require(!closed)

        val writer = filerWriter
        if (writer != null) {
            writer.write(cbuf, off, len)
        } else {
            // Compare if the given input equals to the existing content
            val old = requireNotNull(oldContent)
            val oldRemaining = old.size - cursor
            if (oldRemaining >= len && arrayEqual(old, cursor, cbuf, off, len)) {
                cursor += len
            } else {
                // There are differences! Start writing to file
                val w = outFile.writer()
                didUpdateFile = true
                // First we write content that matches so far
                w.write(old, 0, cursor)
                // Then set the state to use real file writer
                oldContent = null
                filerWriter = w
                // Then write new stuff
                w.write(cbuf, off, len)
            }
        }
    }

    override fun flush() {
        filerWriter?.flush()
    }

    override fun close() {
        closed = true
        filerWriter?.close()
        filerWriter = null
    }

    companion object {
        private fun arrayEqual(c1: CharArray, off1: Int, c2: CharArray, off2: Int, len: Int): Boolean {
            for (i in 0 until len) {
                if (c1[off1 + i] != c2[off2 + i]) {
                    return false
                }
            }

            return true
        }
    }
}