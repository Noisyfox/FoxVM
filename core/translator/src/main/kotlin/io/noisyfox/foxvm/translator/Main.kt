package io.noisyfox.foxvm.translator

import picocli.CommandLine
import java.io.File
import java.util.concurrent.Callable
import kotlin.system.exitProcess

@CommandLine.Command(
    description = ["Translate the given jars to C source codes."],
    name = "translator",
    mixinStandardHelpOptions = true,
    version = ["0.1"],
    sortOptions = false
)
object TranslatorCommand : Callable<Int> {

    @CommandLine.Option(
        names = ["-r", "--runtimeClasspath"],
        split = "\${sys:path.separator}",
        description = [
            "Runtime classpath that won't be translated.",
            "A \${sys:path.separator} separated list of directories, JAR archives, and ZIP archives to search for class files."
        ],
        required = false
    )
    var runtimeClasspath: Array<File>? = null

    @CommandLine.Option(
        names = ["-i", "--inClasspath"],
        split = "\${sys:path.separator}",
        description = [
            "Input classpath that need to be translated.",
            "A \${sys:path.separator} separated list of directories, JAR archives, and ZIP archives to search for class files."
        ],
        required = true
    )
    lateinit var inClasspath: Array<File>

    @CommandLine.Option(
        names = ["-o", "--out"],
        description = ["Path that the translated code files will be wrote to."],
        required = true
    )
    lateinit var outputPath: File

    override fun call(): Int {
        Translator(runtimeClasspath ?: emptyArray(), inClasspath, outputPath).execute()

        return 0
    }
}

fun main(args: Array<String>) {
    val exitCode = CommandLine(TranslatorCommand).execute(*args)
    exitProcess(exitCode)
}
