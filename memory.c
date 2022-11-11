#include <stdlib.h>
#include <assert.h>
#include "memory.h"

void initMemory(FreeList* freeList, size_t size) {
    void* allocation = malloc(size);
    assert(allocation && size >= sizeof(Block));

    Block* block = (Block*) allocation;
    // TODO this loses 16 bytes of capacity for probably no reason since the size of the block gets reclaimed when the pointer is handed out
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
    void* result = NULL;

    Block** prevPtr = &freeList->first;
    for (Block* block = freeList->first; block; block = block->next) {
        if (block->blockSize > newSize) {
            Block* newBlock = (Block *)((void * ) (block) + newSize);
            newBlock->blockSize = block->blockSize - newSize;
            newBlock->next = block->next;
            *prevPtr = newBlock;
            result = (void*) block;
            break;
        }
        if (block->blockSize == newSize) {
            *prevPtr = block->next;
            result = (void*) block;
            break;
        }
        prevPtr = &block->next;
    }

    if (pointer && oldSize) {
        Block* last = freeList->first;
        while(last->next) {
            last = last->next;
        }
        Block* newBlock = (Block*) pointer;
        newBlock->next = NULL;
        newBlock->blockSize = oldSize;
        last->next = newBlock;
    }

    return result;
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
