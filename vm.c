#include <stdio.h>
#include <assert.h>
#include "common.h"
#include "vm.h"
#include "debug.h"
#include "compiler.h"

static void resetStack(VM* vm) {
    vm->stack.count = 0;
}

void initVM(VM* vm) {
    initValueArray(&vm->stack);
    resetStack(vm);
}

void freeVM(FreeList* freeList, VM* vm) {
    freeValueArray(freeList, &vm->stack);
}

static InterpretResult run(FreeList* freeList, VM* vm) {
#define READ_BYTE (*vm->ip++)
#define PEEK (vm->stack.values[vm->stack.count - 1])
#define BINARY_OP(op) do { Value b = pop(vm); PEEK = PEEK op b; } while (false)

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
            case OP_RETURN:
                printValue(pop(vm));
                printf("\n");
                return INTERPRET_OK;
            case OP_ADD: {
                BINARY_OP(+);
                break;
            }
            case OP_SUBTRACT: {
                BINARY_OP(-);
                break;
            }
            case OP_MULTIPLY: {
                BINARY_OP(*);
                break;
            }
            case OP_DIVIDE: {
                BINARY_OP(/);
                break;
            }
            case OP_NEGATE: {
                PEEK = -PEEK;
                break;
            }
            case OP_CONSTANT: {
                Value constant = vm->chunk->constants.values[READ_BYTE];
                push(freeList, vm, constant);
                break;
            }
            case OP_CONSTANT_LONG: {
                uint32_t index = (READ_BYTE << 16) | (READ_BYTE << 8) | READ_BYTE;
                Value constant = vm->chunk->constants.values[index];
                push(freeList, vm, constant);
                break;
            }
        }
    }

    return INTERPRET_OK;

#undef READ_BYTE
#undef PEEK
#undef BINARY_OP
}

InterpretResult interpret(FreeList* freeList, VM* vm, const char* source) {
    compile(source);
    return INTERPRET_OK;
}

void push(FreeList* freeList, VM* vm, Value value) {
    writeValue(freeList, &vm->stack, value);
}

Value pop(VM* vm) {
    if (vm->stack.count <= 0) assert(!"Empty stack");

    return vm->stack.values[--vm->stack.count];
}

