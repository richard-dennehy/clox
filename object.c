#include <string.h>
#include <stdio.h>
#include "object.h"

static Obj* allocateObject(FreeList* freeList, size_t size, ObjType type) {
    Obj* object = (Obj*) reallocate(freeList, NULL, 0, size);
    object->type = type;
    return object;
}
#define ALLOCATE_OBJ(type, objectType) (type*) allocateObject(freeList, sizeof(type), objectType)

static ObjString* allocateString(FreeList* freeList, char* chars, uint32_t length) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    return string;
}

ObjString* copyString(FreeList* freeList, const char* chars, uint32_t length) {
    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(freeList, heapChars, length);
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
    }
}

ObjString* takeString(FreeList* freeList, char* chars, uint32_t length) {
    return allocateString(freeList, chars, length);
}
