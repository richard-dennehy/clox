#include <stdio.h>
#include "common.h"
#include "vm.h"
#include "debug.h"

void initVM(VM* vm) {

}

void freeVM(VM* vm) {

}

static InterpretResult run(VM* vm) {
#define READ_BYTE (*vm->ip++)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        disassembleInstruction(vm->chunk, (uint32_t) (vm->ip - vm->chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE) {
            case OP_RETURN:
                return INTERPRET_OK;
            case OP_CONSTANT: {
                Value constant = vm->chunk->constants.values[READ_BYTE];
                printValue(constant);
                printf("\n");
                break;
            }
            case OP_CONSTANT_LONG: {
                uint32_t index = (READ_BYTE << 16) | (READ_BYTE << 8) | READ_BYTE;
                Value constant = vm->chunk->constants.values[index];
                printValue(constant);
                printf("\n");
                break;
            }
        }
    }

#undef READ_BYTE
}

InterpretResult interpret(VM* vm, Chunk* chunk) {
    vm->chunk = chunk;
    vm->ip = vm->chunk->code;
    return run(vm);
}
