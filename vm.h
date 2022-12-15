#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"

typedef struct {
    Chunk* chunk;
    uint8_t* ip;
    ValueArray stack;
    Obj* objects;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

void initVM(VM* vm);
void freeVM(FreeList* freeList, VM* vm);
InterpretResult interpret(FreeList* freeList, VM* vm, const char* source);
void push(FreeList* freeList, VM* vm, Value value);
Value pop(VM* vm);

#endif //CLOX_VM_H
