#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include "common.h"
#include "vm.h"
#include "debug.h"
#include "compiler.h"
#include "object.h"

static void resetStack(VM* vm) {
    vm->stack.count = 0;
}

void initVM(VM* vm) {
    initValueArray(&vm->stack);
    resetStack(vm);
    vm->objects = NULL;
}

void freeVM(FreeList* freeList, VM* vm) {
    freeValueArray(freeList, &vm->stack);
}

static void runtimeError(VM* vm, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm->ip - vm->chunk->code - 1;
    uint32_t line = getLine(vm->chunk, instruction);
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack(vm);
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate(FreeList* freeList, VM* vm) {
    ObjString* b = AS_STRING(pop(vm));
    ObjString* a = AS_STRING(pop(vm));

    uint32_t length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(freeList, chars, length);
    push(freeList, vm, OBJ_VAL(result));
}

static InterpretResult run(FreeList* freeList, VM* vm) {
#define READ_BYTE (*vm->ip++)
#define PEEK(distance) (vm->stack.values[vm->stack.count - 1 - distance])
#define BINARY_OP(valueType, op) do { \
    if (!IS_NUMBER(PEEK(0)) || !IS_NUMBER(PEEK(1))) { \
        runtimeError(vm, "Operands must be numbers.");\
        return INTERPRET_RUNTIME_ERROR; \
    }\
    double b = AS_NUMBER(pop(vm));    \
    double a = AS_NUMBER(PEEK(0));    \
    PEEK(0) = valueType(a op b);\
} while (false)
#define PUSH(value) push(freeList, vm, value)

    while (vm->ip < vm->chunk->code + vm->chunk->count) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (uint32_t i = 0; i < vm->stack.count; i++) {
            printf("[");
            printValue(vm->stack.values[i]);
            printf("]");
        }
        printf("\n");
        disassembleInstruction(vm->chunk, (uint32_t) (vm->ip - vm->chunk->code));
#endif
        switch (READ_BYTE) {
            case OP_RETURN:
                printValue(pop(vm));
                printf("\n");
                return INTERPRET_OK;
            case OP_ADD: {
                if (IS_STRING(PEEK(0)) && IS_STRING(PEEK(1))) {
                    concatenate(freeList, vm);
                } else if (IS_NUMBER(PEEK(0)) && IS_NUMBER(PEEK(1))) {
                    double b = AS_NUMBER(pop(vm));
                    double a = AS_NUMBER(PEEK(0));
                    PEEK(0) = NUMBER_VAL(a + b);
                } else {
                    runtimeError(vm, "Operands must be two numbers or two strings");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: {
                BINARY_OP(NUMBER_VAL, -);
                break;
            }
            case OP_MULTIPLY: {
                BINARY_OP(NUMBER_VAL, *);
                break;
            }
            case OP_DIVIDE: {
                BINARY_OP(NUMBER_VAL, /);
                break;
            }
            case OP_NEGATE: {
                if (!IS_NUMBER(PEEK(0))) {
                    runtimeError(vm, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                PUSH(NUMBER_VAL(-AS_NUMBER(pop(vm))));
                break;
            }
            case OP_CONSTANT: {
                PUSH(vm->chunk->constants.values[READ_BYTE]);
                break;
            }
            case OP_CONSTANT_LONG: {
                uint32_t index = (READ_BYTE << 16) | (READ_BYTE << 8) | READ_BYTE;
                PUSH(vm->chunk->constants.values[index]);
                break;
            }
            case OP_NIL: {
                PUSH(NIL_VAL);
                break;
            }
            case OP_TRUE: {
                PUSH(BOOL_VAL(true));
                break;
            }
            case OP_FALSE: {
                PUSH(BOOL_VAL(false));
                break;
            }
            case OP_NOT: {
                PUSH(BOOL_VAL(isFalsey(pop(vm))));
                break;
            }
            case OP_EQUAL: {
                Value b = pop(vm);
                PEEK(0) = BOOL_VAL(valuesEqual(PEEK(0), b));
                break;
            }
            case OP_GREATER: {
                BINARY_OP(BOOL_VAL, >);
                break;
            }
            case OP_LESS: {
                BINARY_OP(BOOL_VAL, <);
                break;
            }
        }
    }

    return INTERPRET_OK;

#undef PUSH
#undef READ_BYTE
#undef PEEK
#undef BINARY_OP
}

InterpretResult interpret(FreeList* freeList, VM* vm, const char* source) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(freeList, source, &chunk)) {
        freeChunk(freeList, &chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm->chunk = &chunk;
    vm->ip = chunk.code;

    InterpretResult result = run(freeList, vm);

    freeChunk(freeList, &chunk);
    return result;
}

void push(FreeList* freeList, VM* vm, Value value) {
    writeValue(freeList, &vm->stack, value);
}

Value pop(VM* vm) {
    if (vm->stack.count <= 0) assert(!"Empty stack");

    return vm->stack.values[--vm->stack.count];
}

