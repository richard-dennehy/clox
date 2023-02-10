#include <string.h>
#include <stdio.h>
#include "object.h"

static Obj* allocateObject(VM* vm, size_t size, ObjType type) {
    Obj* object = (Obj*) reallocate(vm->freeList, NULL, 0, size);
    object->type = type;
    object->next = vm->objects;
    vm->objects = object;
    return object;
}
#define ALLOCATE_OBJ(type, objectType) (type*) allocateObject(vm, sizeof(type), objectType)

static ObjString* allocateString(VM* vm, char* chars, uint32_t length, uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    tableSet(&vm->strings, string, NIL_VAL);
    return string;
}

static uint32_t hashString(const char* key, uint32_t length) {
    // FNV-1a
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; ++i) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString* copyString(VM* vm, const char* chars, uint32_t length) {
    FreeList* freeList = vm->freeList;
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm->strings, chars, length, hash);
    if (interned) return interned;

    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(vm, heapChars, length, hash);
}

ObjFunction* newFunction(VM* vm) {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    initChunk(&function->chunk);

    return function;
}

ObjNative* newNative(VM* vm, NativeFn function, uint8_t arity) {
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
        case OBJ_FUNCTION:
            printFunction(print, AS_FUNCTION(value));
            break;
        case OBJ_NATIVE:
            print("<native fn>");
            break;
        case OBJ_STRING:
            print("%s", AS_CSTRING(value));
            break;
    }
}

ObjString* takeString(VM* vm, char* chars, uint32_t length) {
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm->strings, chars, length, hash);
    if (interned) {
        FreeList* freeList = vm->freeList;
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    return allocateString(vm, chars, length, hash);
}

static void freeObject(FreeList* freeList, Obj* object) {
    switch(object->type) {
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*) object;
            freeChunk(freeList, &function->chunk);
            FREE(ObjFunction, object);
            break;
        }
        case OBJ_NATIVE: {
            FREE(ObjNative, object);
            break;
        }
        case OBJ_STRING: {
            ObjString* string = (ObjString*) object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
    }
}

void freeObjects(VM* vm) {
    Obj* object = vm->objects;
    while (object) {
        Obj* next = object->next;
        freeObject(vm->freeList, object);
        object = next;
    }
}
