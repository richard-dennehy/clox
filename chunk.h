#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"
#include "memory.h"

typedef enum {
    OP_CONSTANT,
    OP_CONSTANT_LONG,
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
void writeConstant(FreeList* freeList, Chunk* chunk, Value value, uint32_t line);
uint32_t getLine(Chunk* chunk, uint32_t instructionIndex);

#endif //CLOX_CHUNK_H
