#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h"

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*) reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))
#define FREE_ARRAY(type, pointer, oldCount) \
    (type*) reallocate(pointer, sizeof(type) * (oldCount), 0)

typedef struct Block {
    struct Block* next;
    size_t blockSize;
} Block;

typedef struct {
    Block* first;
    void* ptr_;
} FreeList;

void initMemory(FreeList* freeList, size_t size);
void freeMemory(FreeList* freeList);
void* newReallocate(FreeList* freeList, void* pointer, size_t oldSize, size_t newSize);
void* reallocate(void* pointer, size_t oldSize, size_t newSize);

#endif //CLOX_MEMORY_H
