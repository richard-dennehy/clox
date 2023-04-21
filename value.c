#include <string.h>
#include <assert.h>
#include "value.h"
#include "object.h"

void initValueArray(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValue(VM* vm, ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        uint32_t oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(array->capacity);
        array->values = VM_GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }

    array->values[array->count++] = value;
}

void freeValueArray(VM* vm, ValueArray* array) {
    VM_FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
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

void markValue(Value value) {
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}
