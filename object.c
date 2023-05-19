#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <assert.h>
#include "object.h"

static Obj* allocateObject(VM* vm, Compiler* compiler, size_t size, ObjType type) {
    Obj* object = (Obj*) reallocate(vm, compiler, NULL, 0, size);
    object->type = type;
    object->isMarked = false;
    object->next = vm->objects;
    vm->objects = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

    return object;
}
#define ALLOCATE_OBJ(type, objectType) (type*) allocateObject(vm, compiler, sizeof(type), objectType)

static ObjString* allocateString(VM* vm, Compiler* compiler, char* chars, uint32_t length, uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    // make new string visible to GC
    writeValue(vm, compiler, &vm->stack, OBJ_VAL(string));
    tableSet(vm, compiler, &vm->strings, string, NIL_VAL);
    pop(vm);
    return string;
}

static uint32_t hashString(const char* key, uint32_t length) {
    // FNV-1a
    uint32_t hash = 2166136261u;
    for (uint32_t i = 0; i < length; ++i) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString* copyString(VM* vm, Compiler* compiler, const char* chars, uint32_t length) {
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm->strings, chars, length, hash);
    if (interned) return interned;

    char* heapChars = COMPILER_ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(vm, compiler, heapChars, length, hash);
}

ObjFunction* newFunction(VM* vm, Compiler* compiler) {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    // GC shenanigans
    writeValue(vm, compiler, &vm->stack, OBJ_VAL(function));
    initChunk(vm, compiler, &function->chunk);
    pop(vm);

    return function;
}

ObjClosure* newClosure(VM* vm, Compiler* compiler, ObjFunction* function) {
    ObjUpvalue** upvalues = COMPILER_ALLOCATE(ObjUpvalue*, function->upvalueCount);

    for (uint32_t i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

ObjNative* newNative(VM* vm, Compiler* compiler, NativeFn function, uint8_t arity) {
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    native->arity = arity;
    return native;
}

static void printFunction(Printer* print, ObjFunction* function) {
    if (function->name) {
        print("<fn %s>", function->name->chars);
    } else {
        print("<script>");
    }
}

void printObject(Printer* print, Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_CLOSURE: {
            printFunction(print, AS_CLOSURE(value)->function);
            break;
        }
        case OBJ_FUNCTION:
            printFunction(print, AS_FUNCTION(value));
            break;
        case OBJ_NATIVE:
            print("<native fn>");
            break;
        case OBJ_STRING:
            print("%s", AS_CSTRING(value));
            break;
        case OBJ_UPVALUE:
            print("upvalue");
            break;
        case OBJ_NONE:
            assert(!"Use after free");
    }
}

ObjString* takeString(VM* vm, Compiler* compiler, char* chars, uint32_t length) {
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm->strings, chars, length, hash);
    if (interned) {
        VM_FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    return allocateString(vm, compiler, chars, length, hash);
}

ObjUpvalue* newUpvalue(VM* vm, Compiler* compiler, Value* slot) {
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->location = slot;
    upvalue->next = NULL;
    upvalue->closed = NIL_VAL;
    return upvalue;
}

void freeObject(VM* vm, Obj* object) {
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)object, object->type);
#endif

    ObjType type = object->type;
    object->type = OBJ_NONE;

    switch(type) {
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*) object;
            VM_FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
            VM_FREE(ObjClosure, object);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*) object;
            freeChunk(vm, &function->chunk);
            VM_FREE(ObjFunction, object);
            break;
        }
        case OBJ_NATIVE: {
            VM_FREE(ObjNative, object);
            break;
        }
        case OBJ_STRING: {
            ObjString* string = (ObjString*) object;
            VM_FREE_ARRAY(char, string->chars, string->length + 1);
            VM_FREE(ObjString, object);
            break;
        }
        case OBJ_UPVALUE: {
            VM_FREE(ObjUpvalue, object);
            break;
        }
        case OBJ_NONE: {
            assert(!"Double free");
        }
    }
}

void freeObjects(VM* vm) {
    Obj* object = vm->objects;
    while (object) {
        Obj* next = object->next;
        freeObject(vm, object);
        object = next;
    }
}

void markObject(VM* vm, Obj* object) {
    if (!object) return;
    if (object->isMarked) return;

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*) object);
    printValue(printf, OBJ_VAL(object));
    printf("\n");
#endif
    object->isMarked = true;

    if (vm->greyCapacity < vm->greyCount + 1) {
        vm->greyCapacity = GROW_CAPACITY(vm->greyCapacity);
        // using system allocator as the custom allocator depends on this
        Obj** newStack = (Obj**) realloc(vm->greyStack, sizeof(Obj*) * vm->greyCapacity);

        // OOM
        if (!newStack) {
            exit(1);
        } else {
            vm->greyStack = newStack;
        }
    }

    vm->greyStack[vm->greyCount++] = object;
}
