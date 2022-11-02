#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
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
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, uint32_t line);
uint32_t getLine(Chunk* chunk, uint32_t instructionIndex);
int32_t addConstant(Chunk* chunk, Value value);

#endif //CLOX_CHUNK_H
