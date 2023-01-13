#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"
#include "memory.h"

typedef enum {
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_DEFINE_GLOBAL,
    OP_DEFINE_GLOBAL_LONG,
    OP_GET_GLOBAL,
    OP_GET_GLOBAL_LONG,
    OP_SET_GLOBAL,
    OP_SET_GLOBAL_LONG,
    OP_GET_LOCAL,
    OP_GET_LOCAL_LONG,
    OP_SET_LOCAL,
    OP_SET_LOCAL_LONG,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_POP,
    OP_PRINT,
    OP_RETURN,
} OpCode;

typedef struct Line {
    uint32_t lineNumber;
    uint32_t instructions;
    struct Line* next;
} Line;

typedef struct {
    uint32_t count;
    uint32_t capacity;
    uint8_t* code;
    Line* firstLine;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);

void freeChunk(FreeList* freeList, Chunk* chunk);

void writeChunk(FreeList* freeList, Chunk* chunk, uint8_t byte, uint32_t line);

uint32_t getLine(Chunk* chunk, uint32_t instructionIndex);

static inline bool isWide(OpCode op) {
    return op == OP_GET_GLOBAL_LONG || op == OP_SET_GLOBAL_LONG || op == OP_CONSTANT_LONG ||
           op == OP_DEFINE_GLOBAL_LONG || op == OP_GET_LOCAL_LONG || op == OP_SET_LOCAL_LONG;
}

#endif //CLOX_CHUNK_H
