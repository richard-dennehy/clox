#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"
#include "memory.h"

typedef double Value;

typedef struct {
    uint32_t capacity;
    uint32_t count;
    Value* values;
} ValueArray;

void initValueArray(ValueArray* array);
void writeValue(FreeList* freeList, ValueArray* array, Value value);
void freeValueArray(FreeList* freeList, ValueArray* array);
void printValue(Value value);

#endif //CLOX_VALUE_H
