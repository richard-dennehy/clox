#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_RETURN,
} OpCode;

typedef struct {
    uint32_t count;
    uint32_t capacity;
    uint8_t* code;
    uint32_t* lines;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, uint32_t line);
int32_t addConstant(Chunk* chunk, Value value);

#endif //CLOX_CHUNK_H
