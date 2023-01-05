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

void initVM(FreeList* freeList, VM* vm) {
    vm->freeList = freeList;
    initValueArray(&vm->stack);
    resetStack(vm);
    vm->objects = NULL;
    initTable(freeList, &vm->globals);
    initTable(freeList, &vm->strings);
}

void freeVM(VM* vm) {
    freeTable(&vm->globals);
    freeTable(&vm->strings);
    freeValueArray(vm->freeList, &vm->stack);
    freeObjects(vm);
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

static void concatenate(VM* vm) {
    FreeList* freeList = vm->freeList;
    ObjString* b = AS_STRING(pop(vm));
    ObjString* a = AS_STRING(pop(vm));

    uint32_t length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(vm, chars, length);
    push(vm, OBJ_VAL(result));
}

static InterpretResult run(VM* vm) {
#define READ_BYTE (*vm->ip++)
#define READ_LONG ((READ_BYTE << 16) | (READ_BYTE << 8) | READ_BYTE)
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
        uint8_t instruction;
        switch (instruction = READ_BYTE) {
            case OP_PRINT:
                printValue(pop(vm));
                printf("\n");
                break;
            case OP_POP:
                pop(vm);
                break;
            case OP_ADD: {
                if (IS_STRING(PEEK(0)) && IS_STRING(PEEK(1))) {
                    concatenate(vm);
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
                push(vm, NUMBER_VAL(-AS_NUMBER(pop(vm))));
                break;
            }
            case OP_CONSTANT:
            case OP_CONSTANT_LONG: {
                uint32_t index = isWide(instruction) ? READ_LONG : READ_BYTE;
                push(vm, vm->chunk->constants.values[index]);
                break;
            }
            case OP_DEFINE_GLOBAL:
            case OP_DEFINE_GLOBAL_LONG: {
                uint32_t index = isWide(instruction) ? READ_LONG : READ_BYTE;
                ObjString* name = AS_STRING(vm->chunk->constants.values[index]);
                tableSet(&vm->globals, name, PEEK(0));
                pop(vm);
                break;
            }
            case OP_GET_GLOBAL:
            case OP_GET_GLOBAL_LONG: {
                uint32_t index = isWide(instruction) ? READ_LONG : READ_BYTE;
                ObjString* name = AS_STRING(vm->chunk->constants.values[index]);
                Value value;
                if (!tableGet(&vm->globals, name, &value)) {
                    runtimeError(vm, "Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(vm, value);
                break;
            }
            case OP_SET_GLOBAL:
            case OP_SET_GLOBAL_LONG: {
                uint32_t index = isWide(instruction) ? READ_LONG : READ_BYTE;
                ObjString* name = AS_STRING(vm->chunk->constants.values[index]);
                if (tableSet(&vm->globals, name, PEEK(0))) {
                    tableDelete(&vm->globals, name);
                    runtimeError(vm, "Undefined variable '%s'", name);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_NIL: {
                push(vm, NIL_VAL);
                break;
            }
            case OP_TRUE: {
                push(vm, BOOL_VAL(true));
                break;
            }
            case OP_FALSE: {
                push(vm, BOOL_VAL(false));
                break;
            }
            case OP_NOT: {
                push(vm, BOOL_VAL(isFalsey(pop(vm))));
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

#undef READ_BYTE
#undef READ_LONG
#undef PEEK
#undef BINARY_OP
}

InterpretResult interpret(VM* vm, const char* source) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(vm, source, &chunk)) {
        freeChunk(vm->freeList, &chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm->chunk = &chunk;
    vm->ip = chunk.code;

    InterpretResult result = run(vm);

    freeChunk(vm->freeList, &chunk);
    return result;
}

void push(VM* vm, Value value) {
    writeValue(vm->freeList, &vm->stack, value);
}

Value pop(VM* vm) {
    if (vm->stack.count <= 0) assert(!"Empty stack");

    return vm->stack.values[--vm->stack.count];
}

