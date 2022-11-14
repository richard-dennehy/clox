#include <stdlib.h>
#include <assert.h>
#include "memory.h"

void initMemory(FreeList* freeList, size_t size) {
    void* allocation = malloc(size);
    assert(allocation && size >= sizeof(Block));

    Block* block = (Block*) allocation;
    block->blockSize = size;
    block->next = NULL;

    freeList->first = block;
    freeList->ptr_ = allocation;
}

void freeMemory(FreeList* freeList) {
    free(freeList->ptr_);
    freeList->ptr_ = NULL;
    freeList->first = NULL;
}

// TODO potential improvements: keep list sorted in memory order; merge adjacent free blocks to reduce fragmentation
void* reallocate(FreeList* freeList, void* pointer, size_t oldSize, size_t newSize) {
    void* result = NULL;

    if (newSize) {
        // need to always allocate enough space to be able to reuse the space to store block metadata once it's freed
        if (newSize < sizeof(Block)) newSize = sizeof(Block);

        Block** prevPtr = &freeList->first;
        for (Block* block = freeList->first; block; block = block->next) {
            if (block->blockSize > newSize) {
                Block* newBlock = (Block*) ((void*) (block) + newSize);
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
    }

    if (pointer && oldSize) {
        if (oldSize < sizeof(Block)) oldSize = sizeof(Block);
        Block* last = freeList->first;
        while(last->next) {
            last = last->next;
        }
        if (result) {
            for (size_t i = 0; i < oldSize; i++) {
                uint8_t* src = (uint8_t*) (pointer + i);
                uint8_t* dst = (uint8_t*) (result + i);
                *dst = *src;
            }
        }
        Block* newBlock = (Block*) pointer;
        newBlock->next = NULL;
        newBlock->blockSize = oldSize;
        last->next = newBlock;
    }

    return result;
}
