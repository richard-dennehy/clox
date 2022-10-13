#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"

typedef double Value;

typedef struct {
    uint32_t capacity;
    uint32_t count;
    Value* values;
} ValueArray;

void initValueArray(ValueArray* array);
void writeValue(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);

#endif //CLOX_VALUE_H
