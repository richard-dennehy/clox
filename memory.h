#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h"

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*) reallocate(freeList, pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))
#define FREE_ARRAY(type, pointer, oldCount) \
    (type*) reallocate(freeList, pointer, sizeof(type) * (oldCount), 0)
#define ALLOCATE(type, count) \
    (type*) reallocate(freeList, NULL, 0, sizeof(type) * count)

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
void* reallocate(FreeList* freeList, void* pointer, size_t oldSize, size_t newSize);

#endif //CLOX_MEMORY_H
