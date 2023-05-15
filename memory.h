#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h"
#include "vm.h"

#define GC_HEAP_GROW_FACTOR 2
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

// need to be able to mark compiler objects if GC is triggered while compiling, but don't want to pass NULLs everywhere in the VM code
// note that the compiler(s) are unreachable once compilation is done, so it's fine to implicitly GC any leftover objects from the compilation phase
#define COMPILER_ALLOCATE(type, count) \
    (type*) reallocate(vm, compiler, NULL, 0, sizeof(type) * count)
#define COMPILER_GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*) reallocate(vm, compiler, pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))

typedef struct Block {
    struct Block* next;
    size_t blockSize;
} Block;

struct FreeList {
    Block* first;
    void* base_;
};

void initMemory(FreeList* freeList, size_t size);
void freeMemory(FreeList* freeList);
void* reallocate(VM* vm, Compiler* compiler, void* pointer, size_t oldSize, size_t newSize);
void collectGarbage(VM* vm, Compiler* compiler);
void markRoots(VM* vm);
void markCompilerRoots(VM* vm, Compiler* compiler);

#endif //CLOX_MEMORY_H
