#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "table.h"

#define FRAMES_MAX 64

typedef struct {
    ObjFunction* function;
    uint8_t* ip;
    uint32_t base;
} CallFrame;

typedef struct {
    FreeList* freeList;
    CallFrame frames[FRAMES_MAX];
    uint8_t frameCount;
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
