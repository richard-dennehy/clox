#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include "common.h"
#include "value.h"
#include "vm.h"

typedef enum {
    OBJ_FUNCTION,
    OBJ_STRING,
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
};

struct ObjString {
    Obj obj;
    uint32_t length;
    uint32_t hash;
    char* chars;
};

struct ObjFunction {
    Obj obj;
    uint8_t arity;
    Chunk chunk;
    ObjString* name;
};

ObjString* copyString(VM* vm, const char* chars, uint32_t length);
ObjString* takeString(VM* vm, char* chars, uint32_t length);
ObjFunction* newFunction(VM* vm);
void printObject(Printer* print, Value value);
void freeObjects(VM* vm);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)
#define AS_STRING(value) ((ObjString*) AS_OBJ(value))
#define AS_CSTRING(value) (AS_STRING(value)->chars)

#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define AS_FUNCTION(value) ((ObjFunction*) AS_OBJ(value))

#endif //CLOX_OBJECT_H
