#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "chunk.h"
#include "scanner.h"
#include "vm.h"

struct Compiler;

typedef struct {
    Token current;
    Token previous;
    Scanner* scanner;
    VM* vm;
    struct Compiler* compiler;
    FreeList* freeList;
    bool hadError;
    bool panicMode;
} Parser;

ObjFunction* compile(VM* vm, const char* source);

#endif //CLOX_COMPILER_H
