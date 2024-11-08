#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include "common.h"
#include "chunk.h"
#include "value.h"
#include "vm.h"

typedef enum {
    OBJ_NONE,
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE,
} ObjType;

struct Obj {
    ObjType type;
    bool isMarked;
    struct Obj* next;
};

struct ObjString {
    Obj obj;
    uint32_t length;
    uint32_t hash;
    char* chars;
};

struct ObjUpvalue {
    Obj obj;
    // for open values (i.e. the closed variable is reachable elsewhere)
    Value* location;
    // for closed upvalues (i.e. the closed variable can't be reached elsewhere and will probably get GC'd when the closure is unreachable)
    Value closed;
    ObjUpvalue* next;
};

struct ObjFunction {
    Obj obj;
    uint8_t arity;
    uint32_t upvalueCount;
    Chunk chunk;
    ObjString* name;
};

struct ObjClosure {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    uint32_t upvalueCount;
};

typedef bool (*NativeFn)(VM* vm, Value* out, Value* args);

struct ObjNative {
    Obj obj;
    NativeFn function;
    uint8_t arity;
};

struct ObjClass {
    Obj obj;
    ObjString* name;
    // TODO could probably make constructors faster by directly storing init method here
    Table methods;
};

struct ObjInstance {
    Obj obj;
    ObjClass* class;
    Table fields;
};

struct ObjBoundMethod {
    Obj obj;
    Value receiver;
    ObjClosure* method;
};

ObjString* copyString(VM* vm, Compiler* compiler, const char* chars, uint32_t length);
ObjString* takeString(VM* vm, Compiler* compiler, char* chars, uint32_t length);
ObjUpvalue* newUpvalue(VM* vm, Compiler* compiler, Value* slot);
ObjFunction* newFunction(VM* vm, Compiler* compiler);
ObjBoundMethod* newBoundMethod(VM* vm, Compiler* compiler, Value receiver, ObjClosure* method);
ObjClass* newClass(VM* vm, Compiler* compiler, ObjString* name);
ObjClosure* newClosure(VM* vm, Compiler* compiler, ObjFunction* objFunction);
ObjInstance* newInstance(VM* vm, Compiler* compiler, ObjClass* class);
ObjNative* newNative(VM* vm, Compiler* compiler, NativeFn function, uint8_t arity);
void printObject(Printer* print, Value value);
void freeObjects(VM* vm);
void freeObject(VM* vm, Obj* object);
void markObject(VM* vm, Obj* object);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)
#define AS_STRING(value) ((ObjString*) AS_OBJ(value))
#define AS_CSTRING(value) (AS_STRING(value)->chars)

#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value) isObjType(value, OBJ_CLASS)
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
#define AS_BOUND_METHOD(value) ((ObjBoundMethod*) AS_OBJ(value))
#define AS_CLASS(value) ((ObjClass*) AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure*) AS_OBJ(value))
#define AS_FUNCTION(value) ((ObjFunction*) AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjInstance*) AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative*) AS_OBJ(value)))

#endif //CLOX_OBJECT_H
