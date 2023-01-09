#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "table.h"

typedef struct {
    FreeList* freeList;
    Chunk* chunk;
    uint8_t* ip;
    ValueArray stack;
    Table globals;
    Table strings;
    Obj* objects;
    Printer* print;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

void initVM(FreeList* freeList, VM* vm);
void freeVM(VM* vm);
InterpretResult interpret(VM* vm, const char* source);
void push(VM* vm, Value value);
Value pop(VM* vm);

#endif //CLOX_VM_H
