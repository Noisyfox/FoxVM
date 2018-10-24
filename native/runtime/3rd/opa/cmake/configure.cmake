# Build args

# Platform checks
include ( CheckFunctionExists )
include ( CheckIncludeFiles )
include ( CheckSymbolExists )
include ( CheckCSourceCompiles )
include ( CheckTypeSize )
include ( CheckSTDC )

#check attributes
check_c_source_compiles("int main(int argc __attribute__((unused)), char** argv __attribute__((unused))) {}" OPA_HAVE_GCC_ATTRIBUTE)

check_include_files ( stddef.h OPA_HAVE_STDDEF_H )

check_symbol_exists ( sched_yield "sched.h" OPA_HAVE_SCHED_YIELD )

check_type_size(int OPA_SIZEOF_INT)
check_type_size(void* OPA_SIZEOF_VOID_P)

# Check platform
MACRO(CHECK_OPA_PRIMITIVE FILE VARIABLE)
    IF(NOT DEFINED ${VARIABLE})
        set (CMAKE_REQUIRED_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include")
        set (CMAKE_REQUIRED_DEFINITIONS -DOPA_SIZEOF_INT=${OPA_SIZEOF_INT} -DOPA_SIZEOF_VOID_P=${OPA_SIZEOF_VOID_P})

        check_c_source_compiles("
        #ifndef _opa_inline
        #define _opa_inline inline
        #endif
        #ifndef _opa_restrict
        #define _opa_restrict restrict
        #endif
        #ifndef _opa_const
        #define _opa_const const
        #endif

        #include \"opa_util.h\" /* for OPA_ATTRIBUTE and friends */
        #include \"primitives/${FILE}\"

        int main(void) {
            OPA_int_t a, b;
            int c;

            OPA_store_int(&a, 0);
            OPA_store_int(&b, 1);
            c = OPA_load_int(&a);

            OPA_add_int(&a, 10);
            OPA_incr_int(&a);
            OPA_decr_int(&a);

            c = OPA_decr_and_test_int(&a);
            c = OPA_fetch_and_add_int(&a, 10);
            c = OPA_fetch_and_incr_int(&a);
            c = OPA_fetch_and_decr_int(&a);

            c = OPA_cas_int(&a, 10, 11);
            c = OPA_swap_int(&a, OPA_load_int(&b));

            OPA_write_barrier();
            OPA_read_barrier();
            OPA_read_write_barrier();

            return 0;
        }" ${VARIABLE})
    ENDIF()
ENDMACRO()

check_c_source_compiles("
    int main(void) {
        __asm__ __volatile__  ( \"ILLEGAL_ASM_STMT\" );
        return 0;
    }" compiler_not_rejects_bogus_asm)

if(NOT compiler_not_rejects_bogus_asm)
    CHECK_OPA_PRIMITIVE(opa_gcc_intel_32_64.h OPA_HAVE_GCC_X86_32_64)
    CHECK_OPA_PRIMITIVE(opa_gcc_intel_32_64_p3.h OPA_HAVE_GCC_X86_32_64_P3)
    CHECK_OPA_PRIMITIVE(opa_gcc_ia64.h OPA_HAVE_GCC_AND_IA64_ASM)
    CHECK_OPA_PRIMITIVE(opa_gcc_ppc.h OPA_HAVE_GCC_AND_POWERPC_ASM)
    CHECK_OPA_PRIMITIVE(opa_gcc_arm.h OPA_HAVE_GCC_AND_ARM_ASM)
    CHECK_OPA_PRIMITIVE(opa_gcc_sicortex.h OPA_HAVE_GCC_AND_SICORTEX_ASM)
endif()

CHECK_OPA_PRIMITIVE(opa_gcc_intrinsics.h OPA_HAVE_GCC_INTRINSIC_ATOMICS)
CHECK_OPA_PRIMITIVE(opa_nt_intrinsics.h OPA_HAVE_NT_INTRINSICS)
CHECK_OPA_PRIMITIVE(opa_sun_atomic_ops.h OPA_HAVE_SUN_ATOMIC_OPS)
