#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h"
#include "vm.h"

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)
#define VM_GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*) reallocate(vm, NULL, pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))
#define VM_FREE_ARRAY(type, pointer, oldCount) \
    (type*) reallocate(vm, NULL, pointer, sizeof(type) * (oldCount), 0)
#define VM_ALLOCATE(type, count) \
    (type*) reallocate(vm, NULL, NULL, 0, sizeof(type) * count)
#define VM_FREE(type, pointer) reallocate(vm, NULL, pointer, sizeof(type), 0)

#define COMPILER_GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*) reallocate(vm, compiler, pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))
#define COMPILER_FREE_ARRAY(type, pointer, oldCount) \
    (type*) reallocate(vm, compiler, pointer, sizeof(type) * (oldCount), 0)
#define COMPILER_ALLOCATE(type, count) \
    (type*) reallocate(vm, compiler, NULL, 0, sizeof(type) * count)
#define COMPILER_FREE(type, pointer) reallocate(vm, compiler, pointer, sizeof(type), 0)

typedef struct Block {
    struct Block* next;
    size_t blockSize;
} Block;

struct FreeList {
    Block* first;
    void* base_;
};

typedef struct Compiler Compiler;

void initMemory(FreeList* freeList, size_t size);
void freeMemory(FreeList* freeList);
void* reallocate(VM* vm, Compiler* compiler, void* pointer, size_t oldSize, size_t newSize);
void collectGarbage(VM* vm, Compiler* compiler);

#endif //CLOX_MEMORY_H
