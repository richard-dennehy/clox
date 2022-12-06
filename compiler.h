#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "chunk.h"
#include "scanner.h"

typedef struct {
    Token current;
    Token previous;
    Scanner* scanner;
    Chunk* compilingChunk;
    FreeList* freeList;
    bool hadError;
    bool panicMode;
} Parser;

bool compile(FreeList* freeList, const char* source, Chunk* chunk);

#endif //CLOX_COMPILER_H
