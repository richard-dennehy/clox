#include <stdlib.h>
#include <assert.h>
#include "memory.h"

void initMemory(FreeList* freeList, size_t size) {
    void* allocation = malloc(size);
    assert(allocation && size >= sizeof(Block));

    Block* block = (Block*) allocation;
    block->blockSize = size - sizeof(Block);
    block->next = NULL;

    freeList->first = block;
    freeList->ptr_ = allocation;
}

void freeMemory(FreeList* freeList) {
    free(freeList->ptr_);
    freeList->ptr_ = NULL;
    freeList->first = NULL;
}

void* newReallocate(FreeList* freeList, void* pointer, size_t oldSize, size_t newSize) {
    Block** prevPtr = &freeList->first;
    for (Block* block = freeList->first; block; block = block->next) {
        if (block->blockSize > newSize) {
            Block* newBlock = (Block *)((void * ) (block) + newSize);
            newBlock->blockSize = block->blockSize - newSize;
            newBlock->next = block->next;
            *prevPtr = newBlock;
            return (void*) block;
        }
        if (block->blockSize == newSize) {
            *prevPtr = block->next;
            return (void*) block;
        }
        prevPtr = &block->next;
    }
    return NULL;
}

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) exit(1);
    return result;
}
