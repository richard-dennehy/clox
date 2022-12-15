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

static ObjString* allocateString(VM* vm, char* chars, uint32_t length) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    return string;
}

ObjString* copyString(VM* vm, const char* chars, uint32_t length) {
    FreeList* freeList = vm->freeList;
    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(vm, heapChars, length);
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
    }
}

ObjString* takeString(VM* vm, char* chars, uint32_t length) {
    return allocateString(vm, chars, length);
}

static void freeObject(FreeList* freeList, Obj* object) {
    switch(object->type) {
        case OBJ_STRING: {
            ObjString* string = (ObjString*) object;
            FREE_ARRAY(char, string->chars, string->length);
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
