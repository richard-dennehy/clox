#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"
#include "memory.h"

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
    } as;
} Value;

// TODO is there a good reason these need to be macros rather than functions?
#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)

typedef struct {
    uint32_t capacity;
    uint32_t count;
    Value* values;
} ValueArray;

void initValueArray(ValueArray* array);
void writeValue(FreeList* freeList, ValueArray* array, Value value);
void freeValueArray(FreeList* freeList, ValueArray* array);
void printValue(Value value);
bool valuesEqual(Value a, Value b);

#endif //CLOX_VALUE_H
