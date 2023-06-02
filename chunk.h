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
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_CLOSURE,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_GET_UPVALUE_LONG,
    OP_SET_UPVALUE_LONG,
    OP_CLOSE_UPVALUE,
    OP_CLASS,
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_METHOD,
    OP_INVOKE,
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

void initChunk(VM* vm, Compiler* compiler, Chunk* chunk);

void freeChunk(VM* vm, Chunk* chunk);

void writeChunk(VM* vm, Compiler* compiler, Chunk* chunk, uint8_t byte, uint32_t line);

uint32_t getLine(Chunk* chunk, uint32_t instructionIndex);

#endif //CLOX_CHUNK_H
