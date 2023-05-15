#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
} ValueType;

typedef struct Obj Obj;
typedef struct ObjString ObjString;
typedef struct ObjFunction ObjFunction;
typedef struct ObjClosure ObjClosure;
typedef struct ObjNative ObjNative;
typedef struct ObjUpvalue ObjUpvalue;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;
} Value;

// TODO is there a good reason these need to be macros rather than functions?
#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object) ((Value){VAL_OBJ, {.obj = (Obj*)object}})

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.obj)

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value.type) == VAL_OBJ)

typedef struct {
    uint32_t capacity;
    uint32_t count;
    Value* values;
} ValueArray;

void initValueArray(VM* vm, Compiler* compiler, ValueArray* array);
void writeValue(VM* vm, Compiler* compiler, ValueArray* array, Value value);
void freeValueArray(VM* vm, ValueArray* array);
void printValue(Printer* print, Value value);
bool valuesEqual(Value a, Value b);
void markValue(VM* vm, Value value);
void markValueArray(VM* vm, ValueArray* array);

#endif //CLOX_VALUE_H
