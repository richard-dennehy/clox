#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"
#include <string.h>

// avoid circular dependency between object.h and value.h
typedef struct Obj Obj;
typedef struct ObjString ObjString;
typedef struct ObjFunction ObjFunction;
typedef struct ObjClosure ObjClosure;
typedef struct ObjNative ObjNative;
typedef struct ObjUpvalue ObjUpvalue;
typedef struct ObjClass ObjClass;
typedef struct ObjInstance ObjInstance;
typedef struct ObjBoundMethod ObjBoundMethod;

#ifdef NAN_BOXING
typedef uint64_t Value;

// "is object" tag
#define SIGN_BIT ((uint64_t)0x8000000000000000)
// tag for non-numeric values - all float64 values with all exponent bits & upper mantissa bit set are quiet NaN
// Intel defines a QNaN Floating-Point Indefinite value using one of the remaining 52 bits, leaving 51 bits to freely abuse for type punning
#define QNAN     ((uint64_t)0x7ffc000000000000)

#define TAG_NIL   1 // 01
#define TAG_FALSE 2 // 10
#define TAG_TRUE  3 // 11

#define AS_NUMBER(value) valueToNumber(value)
#define IS_NUMBER(value) (((value) & QNAN) != QNAN)
#define NUMBER_VAL(number) numberToValue(number)

#define IS_NIL(value) ((value) == NIL_VAL)
#define NIL_VAL ((Value) (uint64_t)(QNAN | TAG_NIL))

#define FALSE_VAL ((Value) (uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL ((Value) (uint64_t)(QNAN | TAG_TRUE))

#define AS_BOOL(value) ((value) == TRUE_VAL)
#define IS_BOOL(value) (((value) | 1) == TRUE_VAL)
#define BOOL_VAL(b) ((b) ? TRUE_VAL : FALSE_VAL)

#define AS_OBJ(value) ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))
#define IS_OBJ(value) (((value) & (SIGN_BIT | QNAN)) == (SIGN_BIT | QNAN))
#define OBJ_VAL(obj) (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t) (obj))

static inline double valueToNumber(Value value) {
    // apparently the redundant memcpy will get optimised away to a basic type pun
    double number;
    memcpy(&number, &value, sizeof(Value));
    return number;
}

static inline Value numberToValue(double number) {
    Value value;
    memcpy(&value, &number, sizeof(double));
    return value;
}

#else

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;
} Value;

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

#endif

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
