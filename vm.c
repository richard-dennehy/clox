#include <stdio.h>
#include <assert.h>
#include "common.h"
#include "vm.h"
#include "debug.h"

static void resetStack(VM* vm) {
    vm->stackTop = vm->stack;
}

void initVM(VM* vm) {
    resetStack(vm);
}

void freeVM(VM* vm) {

}

static InterpretResult run(VM* vm) {
#define READ_BYTE (*vm->ip++)
#define BINARY_OP(op) do { Value b = pop(vm); Value a = pop(vm); push(vm, a op b); } while (false)

    while (vm->ip < vm->chunk->code + vm->chunk->count) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm->stack; slot < vm->stackTop; slot++) {
            printf("[");
            printValue(*slot);
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
                push(vm, -pop(vm));
                break;
            }
            case OP_CONSTANT: {
                Value constant = vm->chunk->constants.values[READ_BYTE];
                push(vm, constant);
                break;
            }
            case OP_CONSTANT_LONG: {
                uint32_t index = (READ_BYTE << 16) | (READ_BYTE << 8) | READ_BYTE;
                Value constant = vm->chunk->constants.values[index];
                push(vm, constant);
                break;
            }
        }
    }

#undef READ_BYTE
#undef BINARY_OP
}

InterpretResult interpret(VM* vm, Chunk* chunk) {
    vm->chunk = chunk;
    vm->ip = vm->chunk->code;
    return run(vm);
}

void push(VM* vm, Value value) {
    if(vm->stackTop >= vm->stack + STACK_MAX) {
        assert(!"Stack overflow");
    }
    *vm->stackTop++ = value;
}

Value pop(VM* vm) {
    if (vm->stackTop == vm->stack) {
        assert(!"Stack underflow (?)");
    }
    return *--vm->stackTop;
}

