#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "chunk.h"
#include "scanner.h"
#include "vm.h"

typedef struct ClassCompiler {
    struct ClassCompiler* enclosing;
} ClassCompiler;

typedef struct {
    Token current;
    Token previous;
    Scanner* scanner;
    VM* vm;
    Compiler* compiler;
    ClassCompiler* currentClass;
    bool hadError;
    bool panicMode;
} Parser;

ObjFunction* compile(VM* vm, const char* source);

#endif //CLOX_COMPILER_H
