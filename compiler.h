#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "chunk.h"
#include "scanner.h"
#include "vm.h"

typedef struct {
    Token current;
    Token previous;
    Scanner* scanner;
    Chunk* compilingChunk;
    VM* vm;
    FreeList* freeList;
    bool hadError;
    bool panicMode;
} Parser;

bool compile(VM* vm, const char* source, Chunk* chunk);

#endif //CLOX_COMPILER_H
