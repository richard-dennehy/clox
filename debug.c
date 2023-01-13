#include <stdio.h>
#include "debug.h"

void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (uint32_t offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

static uint32_t simpleInstruction(const char* name, uint32_t offset) {
    printf("%s\n", name);
    return offset + 1;
}

static uint32_t constantInstruction(const char* name, Chunk* chunk, uint32_t offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    printValue(printf, chunk->constants.values[constant]);
    printf("'\n");

    return offset + 2;
}

static uint32_t longConstantInstruction(const char* name, Chunk* chunk, uint32_t offset) {
    uint32_t constant = (chunk->code[offset + 1] << 16) | (chunk->code[offset + 2] << 8) | chunk->code[offset + 3];
    printf("%-16s %4d '", name, constant);
    printValue(printf, chunk->constants.values[constant]);
    printf("'\n");

    return offset + 4;
}

static uint32_t byteInstruction(const char* name, Chunk* chunk, uint32_t offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static uint32_t jumpInstruction(const char* name, Chunk* chunk, int32_t sign, uint32_t offset) {
    uint16_t jump = (chunk->code[offset + 1] << 8) | (chunk->code[offset + 2]);
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

uint32_t disassembleInstruction(Chunk* chunk, uint32_t offset) {
    printf("%04d ", offset);
    if (offset > 0 && getLine(chunk, offset) == getLine(chunk, offset - 1)) {
        printf("   | ");
    } else {
        printf("%4d ", getLine(chunk, offset));
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_CONSTANT_LONG:
            return longConstantInstruction("OP_CONSTANT_LONG", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL_LONG:
            return longConstantInstruction("OP_DEFINE_GLOBAL_LONG", chunk, offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_GET_GLOBAL_LONG:
            return longConstantInstruction("OP_GET_GLOBAL_LONG", chunk, offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL_LONG:
            return longConstantInstruction("OP_SET_GLOBAL_LONG", chunk, offset);
        case OP_GET_LOCAL:
            return byteInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:
            return byteInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", chunk, 1, offset);
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", chunk, 1, offset);
        case OP_LOOP:
            return jumpInstruction("OP_LOOP", chunk, -1, offset);

        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
