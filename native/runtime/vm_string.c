//
// Created by noisyfox on 2020/9/1.
//

#include "vm_string.h"
#include "vm_thread.h"
#include "classloader/vm_boot_classloader.h"
#include "vm_bytecode.h"
#include "vm_array.h"
#include <stdio.h>

extern JAVA_INT foxvm_constant_pool_rt_count;
extern C_CSTR foxvm_constant_pool_rt[];
extern JAVA_LONG foxvm_constant_pool_rt_init_thread[];
extern JAVA_OBJECT foxvm_constant_pool_rt_obj[];

// A private constructor that does not make a copy of the array. Used internally bu the VM.
// java/lang/String.<init>([CZ)V
JAVA_VOID method_9Pjava_lang6CString_4IINIT_A1C_Z(VM_PARAM_CURRENT_CONTEXT);

/**
 * Determine the length of java char array that can hold the given
 * modified utf8 encoded string.
 */
static JAVA_INT string_unicode_length_of(C_CSTR utf8_str) {
    JAVA_INT count = 0;
    C_CSTR cursor = utf8_str;

    unsigned char x;
    while ((x = *cursor) != 0) {
        count++;

        // jvms8 §4.4.7 The CONSTANT_Utf8_info Structure
        if ((x & 0x80u) == 0) {
            // • Code points in the range '\u0001' to '\u007F' are represented by a single byte:
            //   0 bits 6-0
            //   The 7 bits of data in the byte give the value of the code point represented.
        } else if ((x & 0xE0u) == 0xC0u) {
            // • The null code point ('\u0000') and code points in the range '\u0080' to '\u07FF'
            //   are represented by a pair of bytes x and y :
            //   x: 1 1 0 bits 10-6
            //   y: 1 0 bits 5-0
            cursor++;
            unsigned char y = *cursor;
            assert((y & 0xC0u) == 0x80u);
        } else if ((x & 0xF0u) == 0xE0u) {
            // • Code points in the range '\u0800' to '\uFFFF' are represented by 3 bytes x, y,
            //   and z :
            //   x: 1 1 1 0 bits 15-12
            //   y: 1 0 bits 11-6
            //   z: 1 0 bits 5-0
            cursor++;
            unsigned char y = *cursor;
            assert((y & 0xC0u) == 0x80u);
            cursor++;
            unsigned char z = *cursor;
            assert((z & 0xC0u) == 0x80u);
        } else {
            assert(!"Malformed UTF8 encoded string");
        }

        cursor++;
    }

    return count;
}

static JAVA_VOID string_utf8_to_unicode(C_CSTR utf8_str, JAVA_ARRAY char_array) {
    assert(utf8_str != NULL);
    assert(char_array != (JAVA_ARRAY) JAVA_NULL);

    C_CSTR utf8Cursor = utf8_str;
    JAVA_CHAR *unicodeCursor = array_base(char_array, VM_TYPE_CHAR);

    unsigned char x;
    while ((x = *utf8Cursor) != 0) {
        // jvms8 §4.4.7 The CONSTANT_Utf8_info Structure
        if ((x & 0x80u) == 0) {
            // • Code points in the range '\u0001' to '\u007F' are represented by a single byte:
            //   0 bits 6-0
            //   The 7 bits of data in the byte give the value of the code point represented.
            *unicodeCursor = x;
        } else if ((x & 0xE0u) == 0xC0u) {
            // • The null code point ('\u0000') and code points in the range '\u0080' to '\u07FF'
            //   are represented by a pair of bytes x and y :
            //   x: 1 1 0 bits 10-6
            //   y: 1 0 bits 5-0
            //   The two bytes represent the code point with the value:
            //     ((x & 0x1f) << 6) + (y & 0x3f)
            utf8Cursor++;
            unsigned char y = *utf8Cursor;
            assert((y & 0xC0u) == 0x80u);

            *unicodeCursor = ((x & 0x1fu) << 6) | (y & 0x3fu);
        } else if ((x & 0xF0u) == 0xE0u) {
            // • Code points in the range '\u0800' to '\uFFFF' are represented by 3 bytes x, y,
            //   and z :
            //   x: 1 1 1 0 bits 15-12
            //   y: 1 0 bits 11-6
            //   z: 1 0 bits 5-0
            //   The three bytes represent the code point with the value:
            //     ((x & 0xf) << 12) + ((y & 0x3f) << 6) + (z & 0x3f)
            utf8Cursor++;
            unsigned char y = *utf8Cursor;
            assert((y & 0xC0u) == 0x80u);
            utf8Cursor++;
            unsigned char z = *utf8Cursor;
            assert((z & 0xC0u) == 0x80u);

            *unicodeCursor = ((x & 0xfu) << 12) | ((y & 0x3fu) << 6) | (z & 0x3fu);
        } else {
            assert(!"Malformed UTF8 encoded string");
        }
        utf8Cursor++;
        unicodeCursor++;
    }
}

JAVA_OBJECT string_create_utf8(VM_PARAM_CURRENT_CONTEXT, C_CSTR utf8) {
    stack_frame_start(-1, 4, 2);

    JAVA_INT requiredCharCount = string_unicode_length_of(utf8);
    // Allocate a char[] that can contains the unicode string
    stack_push_int(requiredCharCount);
    bc_newarray("[C");
    bc_store(0, VM_SLOT_OBJECT); // Store in local 0
    // Convert the utf8 to unicode string
    string_utf8_to_unicode(utf8, (JAVA_ARRAY) local_of(0).data.o);
    // Create a instance of java/lang/String
    bc_new(g_classInfo_java_lang_String);
    // Dup the stack
    bc_dup();
    // Push the array to stack
    bc_load(0, VM_SLOT_OBJECT);
    // Push the second arg "share"
    bc_iconst_1();
    // Call the init function java/lang/String.<init>([CZ)V
    bc_invoke_special(method_9Pjava_lang6CString_4IINIT_A1C_Z);
    // Now there should be an instance of java/lang/String at the top of the stack
    // We store it in local 0
    bc_store(0, VM_SLOT_OBJECT);

    JAVA_OBJECT objRef = local_of(0).data.o;

    stack_frame_end();

    return objRef;
}

JAVA_OBJECT string_get_constant(VM_PARAM_CURRENT_CONTEXT, JAVA_INT constant_index) {
    assert(constant_index >= 0);
    assert(constant_index < foxvm_constant_pool_rt_count);

    {
        // First we check if the constant instance has been created
        JAVA_OBJECT objRef = foxvm_constant_pool_rt_obj[constant_index];
        if (objRef != JAVA_NULL) {
            return objRef;
        }
    }

    // Otherwise we need to create one
    stack_frame_start(-1, 1, 2);

    // Lock the String class
    VMStackSlot *clazz_slot = &local_of(0);
    clazz_slot->type = VM_SLOT_OBJECT;
    clazz_slot->data.o = (JAVA_OBJECT) g_class_java_lang_String;
    monitor_enter(vmCurrentContext, clazz_slot);

    // Check if this constant is currently being creating by another thread
    JAVA_LONG initThread;
    while ((initThread = foxvm_constant_pool_rt_init_thread[constant_index]) != 0) {
        if (initThread == vmCurrentContext->threadId) {
            monitor_exit(vmCurrentContext, clazz_slot);

            // Error! we have a recursive constant init, cannot complete!
            assert(!"Constant pool recursive init");
            exit(-3);
        } else {
            // Is initializing by another thread, wait
            monitor_wait(vmCurrentContext, clazz_slot, 0, 0);
        }
    }

    // Check if the constant has been created by another thread
    {
        JAVA_OBJECT objRef = foxvm_constant_pool_rt_obj[constant_index];
        if (objRef != JAVA_NULL) {
            monitor_exit(vmCurrentContext, clazz_slot);

            stack_frame_end();
            return objRef;
        }
    }

    // Mark the constant is initializing by this thread
    foxvm_constant_pool_rt_init_thread[constant_index] = vmCurrentContext->threadId;
    monitor_exit(vmCurrentContext, clazz_slot);

    printf("Initializing constant pool item at %d\n", constant_index);

    // Get the utf-8 encoded string
    C_CSTR utf8Encoded = foxvm_constant_pool_rt[constant_index];
    // Call `string_create_utf8`
    JAVA_OBJECT str = string_create_utf8(vmCurrentContext, utf8Encoded);
    // Since we need to call `monitor_enter()` later, to prevent GC from deleting the created string,
    // we need to keep the reference in the stack frame
    stack_push_object(str);
    // Now there should be an instance of java/lang/String at the top of the stack
    // We store it in local 1
    bc_store(1, VM_SLOT_OBJECT);

    // Now we store the string instance to constant pool
    monitor_enter(vmCurrentContext, clazz_slot);
    JAVA_OBJECT objRef = local_of(1).data.o;
    foxvm_constant_pool_rt_obj[constant_index] = objRef;
    foxvm_constant_pool_rt_init_thread[constant_index] = 0;
    monitor_notify_all(vmCurrentContext, clazz_slot);
    monitor_exit(vmCurrentContext, clazz_slot);

    stack_frame_end();
    return objRef;
}
