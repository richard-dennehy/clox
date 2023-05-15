#include <string.h>
#include <assert.h>
#include "value.h"
#include "object.h"

void initValueArray(VM* vm, Compiler* compiler, ValueArray* array) {
    array->capacity = GROW_CAPACITY(array->capacity);
    array->values = COMPILER_GROW_ARRAY(Value, array->values, 0, array->capacity);
    array->count = 0;
}

void writeValue(VM* vm, Compiler* compiler, ValueArray* array, Value value) {
    assert(array->capacity != 0);
    array->values[array->count++] = value;

    // need to write to array then grow after, otherwise a GC can be triggered while writing temp values to the stack
    if (array->capacity == array->count) {
        uint32_t oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(array->capacity);
        array->values = COMPILER_GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }
}

void freeValueArray(VM* vm, ValueArray* array) {
    VM_FREE_ARRAY(Value, array->values, array->capacity);
    array->count = 0;
    array->capacity = 0;
    array->values = NULL;
}

void printValue(Printer* print, Value value) {
    switch (value.type) {
        case VAL_BOOL:
            print(AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NIL:
            print("nil");
            break;
        case VAL_NUMBER:
            print("%g", AS_NUMBER(value));
            break;
        case VAL_OBJ:
            printObject(print, value);
            break;
    }
}

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL:
            return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:
            return true;
        case VAL_NUMBER:
            return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ:
            return AS_OBJ(a) == AS_OBJ(b);
        default:
            assert(!"Missing switch case");
    }
}

void markValue(VM* vm, Value value) {
    if (IS_OBJ(value)) markObject(vm, AS_OBJ(value));
}

void markValueArray(VM* vm, ValueArray* array) {
    for (uint32_t i = 0; i < array->count; i++) {
        markValue(vm, array->values[i]);
    }
}
