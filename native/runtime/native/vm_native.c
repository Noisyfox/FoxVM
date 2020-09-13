//
// Created by noisyfox on 2020/9/2.
//

#include "vm_native.h"
#include "vm_thread.h"
#include <stdio.h>
#include <stdint.h>
#include "vm_gc.h"
#include "vm_classloader.h"
#include "vm_field.h"
#include "vm_exception.h"
#include "vm_array.h"
#include <string.h>
#include "vm_string.h"

static VMSpinLock g_jniMethodLock = OPA_INT_T_INITIALIZER(0);

static void* g_libHandleThis = NULL;

static struct JNINativeInterface jni;

// A special object that marks the current handle has been deleted
// by `DeleteLocalRef`, `DeleteGlobalRef` or `DeleteWeakGlobalRef`.
// We can't just put NULL in a deleted ref because we use NULL to
// mark a `WeakGlobalRef` that has been reclaimed by GC. That ref
// cannot be reused until `DeleteWeakGlobalRef` has been called.
static JavaObjectBase g_deletedHandle;

// Value that an empty ref table is filled with
// Make sure the bit 0 and bit 32 are 1, which is an invalid address for
// any heap-allocated object.
static const JAVA_OBJECT g_badHandle = (JAVA_OBJECT) UINT64_C(0xABABABABABABABAB);

static void native_reftable_clear(RefTable *table) {
    table->top = 0;
    for (jint i = 0; i < table->capacity; i++) {
        table->objects[i] = g_badHandle;
    }
}

void native_reftable_init(RefTable *table, JAVA_OBJECT* storage, jint capacity) {
    table->next = NULL;
    table->capacity = capacity;
    table->top = 0;
    table->objects = storage;

    native_reftable_clear(table);
}

JAVA_BOOLEAN native_init() {
    spin_lock_init(&g_jniMethodLock);

    g_libHandleThis = native_load_library(NULL);
    assert(g_libHandleThis != NULL);

    return JAVA_TRUE;
}

void native_make_root_stack_frame(NativeStackFrame *root) {
    VMStackFrame *base = &root->baseFrame;

    native_frame_init(root, NULL);

    base->prev = base;
    base->next = base;
}

void native_frame_init(NativeStackFrame *frame, RefTable* initialRefTable) {
    VMStackFrame *base = &frame->baseFrame;

    base->prev = NULL;
    base->next = NULL;
    base->type = VM_STACK_FRAME_NATIVE;
    base->thisClass = NULL;

    // init ref table
    if (initialRefTable) {
        frame->refTable = initialRefTable;
        frame->isJniLocalFrame = JAVA_FALSE;
    } else {
        frame->isJniLocalFrame = JAVA_TRUE;
        frame->refTable = NULL;
    }
}

void native_frame_pop(VM_PARAM_CURRENT_CONTEXT) {
    VMStackFrame *base = stack_frame_top(vmCurrentContext);
    assert(base->type == VM_STACK_FRAME_NATIVE);
    NativeStackFrame *frame = (NativeStackFrame *) base;

    RefTable *table = frame->refTable;
    if (table == NULL) {
        assert(frame->isJniLocalFrame);
        stack_frame_pop(vmCurrentContext);
        return;
    }

    while (table->next != NULL) {
        RefTable *next = table->next;

        // Reset the table
        native_reftable_clear(table);
        // Then free the memory
        heap_free_uncollectable(table);

        table = next;
    }

    // Now deal with the initial table
    // First reset the table
    native_reftable_clear(table);
    if (frame->isJniLocalFrame) {
        // Then free the memory since for jni local frame, all tables are allocated dynamically
        heap_free_uncollectable(table);
        // Reset the frame
        frame->refTable = NULL;
    } else {
        // Leave the table there since the initial table is not allocated dynamically
        frame->refTable = table;
    }

    // Finally pop the frame
    stack_frame_pop(vmCurrentContext);
}

JAVA_VOID native_thread_attach_jni(VM_PARAM_CURRENT_CONTEXT) {
    vmCurrentContext->jni = &jni;
}

void* native_bind_method(VM_PARAM_CURRENT_CONTEXT, MethodInfoNative *method) {
    printf("Resolving native method %s\n", method->longName);

    spin_lock_enter(vmCurrentContext, &g_jniMethodLock);
    // Check again after obtaining the lock
    void *nativePtr = method->nativePtr;
    if (nativePtr) {
        spin_lock_exit(&g_jniMethodLock);
        return nativePtr;
    }

    // The VM checks for a method name match for methods that reside in the native library.
    // The VM looks first for the short name; that is, the name without the argument signature.
    nativePtr = native_find_symbol(g_libHandleThis, method->shortName);
    if (!nativePtr) {
        // It then looks for the long name, which is the name with the argument signature.
        nativePtr = native_find_symbol(g_libHandleThis, method->longName);
    }

    method->nativePtr = nativePtr;
    spin_lock_exit(&g_jniMethodLock);

    if (!nativePtr) {
        fprintf(stderr, "Unable to find native method\n");
        exception_set_UnsatisfiedLinkError(vmCurrentContext, method, "Unable to find native method");
    }

    return nativePtr;
}

JAVA_VOID native_enter_jni(VM_PARAM_CURRENT_CONTEXT) {
    // This cannot be called inside a checkpoint
    assert(!thread_in_saferegion(vmCurrentContext));

    // Enter the saferegion so GC can start working on this thread
    thread_enter_saferegion(vmCurrentContext);
}

JAVA_VOID native_exit_jni(VM_PARAM_CURRENT_CONTEXT) {
    assert(thread_in_saferegion(vmCurrentContext));
    // Leave saferegion so GC won't move any object
    thread_leave_saferegion(vmCurrentContext);
    // Make sure we do left the saferegion
    assert(!thread_in_saferegion(vmCurrentContext));
}

jobject native_get_local_ref(VM_PARAM_CURRENT_CONTEXT, JAVA_OBJECT obj) {
    // This cannot be called inside a saferegion because the obj might be moved by GC
    assert(!thread_in_saferegion(vmCurrentContext));

    VMStackFrame *top = stack_frame_top(vmCurrentContext);
    assert(top->type == VM_STACK_FRAME_NATIVE);
    NativeStackFrame *frame = (NativeStackFrame *) top;

    if (obj == JAVA_NULL) {
        return NULL;
    }

    jint maxCapacity = 8;
    RefTable *table = frame->refTable;
    if (table != NULL) {
        if (table->top < table->capacity) {
            JAVA_OBJECT *ref = &table->objects[table->top];
            table->top++;
            *ref = obj;
            return ref;
        }

        // Find a slot that is marked as g_deletedHandle
        while (table != NULL) {
            maxCapacity = table->capacity > maxCapacity ? table->capacity : maxCapacity;

            for (jint i = 0; i < table->capacity; i++) {
                JAVA_OBJECT *ref = &table->objects[i];
                if (*ref == &g_deletedHandle) {
                    *ref = obj;
                    return ref;
                }
            }
            table = table->next;
        }
    }

    // All existing tables are full, need to allocate a new table
    jint newCapacity = (maxCapacity > 1024 ? 1024 : maxCapacity) * 2;
    table = heap_alloc_uncollectable(sizeof(RefTable) + newCapacity * sizeof(JAVA_OBJECT));
    assert(table);
    native_reftable_init(table, ptr_inc(table, sizeof(RefTable)), newCapacity);

    // Put the table at the beginning of the list
    table->next = frame->refTable;
    frame->refTable = table;

    // Try again
    return native_get_local_ref(vmCurrentContext, obj);
}

JAVA_OBJECT native_dereference(VM_PARAM_CURRENT_CONTEXT, jobject ref) {
    // This cannot be called inside a saferegion because the obj might be moved by GC
    assert(!thread_in_saferegion(vmCurrentContext));

    assert(stack_frame_top(vmCurrentContext)->type == VM_STACK_FRAME_NATIVE);

    if (ref == NULL) {
        return JAVA_NULL;
    }

    JAVA_OBJECT obj = *((JAVA_OBJECT *) ref);
    assert(obj != &g_deletedHandle);

    if (obj == JAVA_NULL) {
        // TODO: assert it's a weak global ref
        return JAVA_NULL;
    }

    // Check if it points to a unused ref
    assert(obj != g_badHandle);

    return obj;
}

// Implementations of JNI interface methods
#define get_thread_context() VM_PARAM_CURRENT_CONTEXT = (VMThreadContext *)env

static ResolvedField *native_get_field(JNIEnv *env, jclass cls, const char *name, const char *sig) {
    assert(cls != NULL);

    get_thread_context();
    native_exit_jni(vmCurrentContext);

    JAVA_CLASS clazz = (JAVA_CLASS) native_dereference(vmCurrentContext, cls);
    assert(clazz != (JAVA_CLASS) JAVA_NULL);

    // GetFieldID()/GetStaticFieldID() causes an uninitialized class to be initialized.
    if (classloader_init_class(vmCurrentContext, clazz) != JAVA_TRUE) {
        // TODO: throw ExceptionInInitializerError
        native_enter_jni(vmCurrentContext);
        return NULL;
    }

    ResolvedField *field = field_find(clazz, name, sig);

    native_enter_jni(vmCurrentContext);
    return field;
}

static jfieldID JNICALL GetFieldID(JNIEnv *env, jclass cls, const char *name, const char *sig) {
    ResolvedField *field = native_get_field(env, cls, name, sig);
    if (field != NULL) {
        // Make sure it's instance field
        if (field_is_static(&field->info)) {
            // TODO: Throw NoSuchFieldError instead
            field = NULL;
        }
    }

    return (jfieldID) field;
}

static jobject JNICALL GetObjectField(JNIEnv *env, jobject obj, jfieldID fieldID) {
    assert(fieldID != NULL);
    ResolvedField *field = (ResolvedField *) fieldID;
    // Make sure it's instance field
    assert(!field_is_static(&field->info));

    get_thread_context();
    native_exit_jni(vmCurrentContext);

    JAVA_OBJECT object = native_dereference(vmCurrentContext, obj);
    if (object == JAVA_NULL) {
        // TODO: throw NullPointerException instead.
        abort();
    }

    JAVA_OBJECT resultObj = *((JAVA_OBJECT *) ptr_inc(object, field->info.offset));
    jobject result = native_get_local_ref(vmCurrentContext, resultObj);

    native_enter_jni(vmCurrentContext);

    return result;
}

static jint JNICALL GetIntField(JNIEnv *env, jobject obj, jfieldID fieldID) {
    assert(fieldID != NULL);
    ResolvedField *field = (ResolvedField *) fieldID;
    // Make sure it's instance field
    assert(!field_is_static(&field->info));

    get_thread_context();
    native_exit_jni(vmCurrentContext);

    JAVA_OBJECT object = native_dereference(vmCurrentContext, obj);
    if (object == JAVA_NULL) {
        // TODO: throw NullPointerException instead.
        abort();
    }

    jint result = *((JAVA_INT *) ptr_inc(object, field->info.offset));

    native_enter_jni(vmCurrentContext);

    return result;
}

static jfieldID JNICALL GetStaticFieldID(JNIEnv *env, jclass cls, const char *name, const char *sig) {
    ResolvedField *field = native_get_field(env, cls, name, sig);
    if (field != NULL) {
        // Make sure it's static field
        if (!field_is_static(&field->info)) {
            // TODO: Throw NoSuchFieldError instead
            field = NULL;
        }
    }

    return (jfieldID) field;
}

static void JNICALL SetStaticObjectField(JNIEnv* env, jclass cls, jfieldID fieldID, jobject value) {
    assert(cls != NULL);
    assert(fieldID != NULL);
    ResolvedField *field = (ResolvedField *) fieldID;
    // Make sure it's static field
    assert(field_is_static(&field->info));

    get_thread_context();
    native_exit_jni(vmCurrentContext);

    JAVA_CLASS clazz = (JAVA_CLASS) native_dereference(vmCurrentContext, cls);
    assert(clazz != (JAVA_CLASS) JAVA_NULL);

    JAVA_OBJECT obj = native_dereference(vmCurrentContext, value);

    *((JAVA_OBJECT *) ptr_inc(clazz, field->info.offset)) = obj;

    native_enter_jni(vmCurrentContext);
}

static jsize JNICALL GetStringUTFLength(JNIEnv *env, jstring string) {
    get_thread_context();
    native_exit_jni(vmCurrentContext);

    jsize result = 0;
    JAVA_OBJECT strObj = native_dereference(vmCurrentContext, string);
    if (strObj == JAVA_NULL) {
        exception_set_NullPointerException(vmCurrentContext, "string");
    } else {
        result = string_utf8_length_of(vmCurrentContext, strObj);
    }

    native_enter_jni(vmCurrentContext);

    return result;
}

static const char* JNICALL GetStringUTFChars(JNIEnv *env, jstring string, jboolean *isCopy) {
    get_thread_context();
    native_exit_jni(vmCurrentContext);

    C_CSTR result = NULL;
    JAVA_OBJECT strObj = native_dereference(vmCurrentContext, string);
    if (strObj == JAVA_NULL) {
        exception_set_NullPointerException(vmCurrentContext, "string");
    } else {
        result = string_utf8_of(vmCurrentContext, strObj);
        if (isCopy && !exception_occurred(vmCurrentContext)) {
            *isCopy = JNI_TRUE;
        }
    }

    native_enter_jni(vmCurrentContext);

    return result;
}

void JNICALL ReleaseStringUTFChars(JNIEnv *env, jstring string, const char* utf) {
    heap_free_uncollectable((void *) utf);
}

static jsize JNICALL GetArrayLength(JNIEnv *env, jarray array) {
    get_thread_context();
    native_exit_jni(vmCurrentContext);

    // TODO: check type and null
    JAVA_ARRAY arrayObj = (JAVA_ARRAY) native_dereference(vmCurrentContext, array);
    if (arrayObj == (JAVA_ARRAY) JAVA_NULL) {
        // TODO: throw NullPointerException instead.
        abort();
    }
    jint length = arrayObj->length;

    native_enter_jni(vmCurrentContext);

    return length;
}

static void JNICALL GetByteArrayRegion(JNIEnv *env, jbyteArray array, jsize start, jsize len, jbyte *buf) {
    get_thread_context();
    native_exit_jni(vmCurrentContext);

    // TODO: check type and null
    JAVA_ARRAY arrayObj = (JAVA_ARRAY) native_dereference(vmCurrentContext, array);
    if (arrayObj == (JAVA_ARRAY) JAVA_NULL) {
        // TODO: throw NullPointerException instead.
        abort();
    }

    // TODO: check bounds

    void *src = array_element_at(arrayObj, VM_TYPE_BYTE, start);
    memcpy(buf, src, len * type_size(VM_TYPE_BYTE));

    native_enter_jni(vmCurrentContext);
}

static struct JNINativeInterface jni = {
        .reserved0 = NULL,
        .reserved1 = NULL,
        .reserved2 = NULL,
        .reserved3 = NULL,
        .GetFieldID = GetFieldID,
        .GetObjectField = GetObjectField,
        .GetIntField = GetIntField,
        .GetStaticFieldID = GetStaticFieldID,
        .SetStaticObjectField = SetStaticObjectField,
        .GetStringUTFLength = GetStringUTFLength,
        .GetStringUTFChars = GetStringUTFChars,
        .ReleaseStringUTFChars = ReleaseStringUTFChars,
        .GetArrayLength = GetArrayLength,
        .GetByteArrayRegion = GetByteArrayRegion,
};
