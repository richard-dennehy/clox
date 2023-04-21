#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "memory.h"

void initMemory(FreeList* freeList, size_t size) {
    void* allocation = malloc(size);
    assert(allocation && size >= sizeof(Block));

    Block* block = (Block*) allocation;
    block->blockSize = size;
    block->next = NULL;

    freeList->first = block;
    freeList->base_ = allocation;
}

void freeMemory(FreeList* freeList) {
    free(freeList->base_);
    freeList->base_ = NULL;
    freeList->first = NULL;
}

// TODO potential improvements: keep list sorted in memory order; merge adjacent free blocks to reduce fragmentation
void* reallocate(VM* vm, Compiler* compiler, void* pointer, size_t oldSize, size_t newSize) {
    uint8_t* result = NULL;

    if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        collectGarbage(vm, compiler);
#endif
    }

    if (newSize) {
        // need to always allocate enough space to be able to reuse the space to store block metadata once it's freed
        if (newSize < sizeof(Block)) newSize = sizeof(Block);

        Block** prevPtr = &vm->freeList->first;
        for (Block* block = vm->freeList->first; block; block = block->next) {
            if (block->blockSize > newSize) {
                Block* newBlock = (Block*) ((uint8_t*) (block) + newSize);
                newBlock->blockSize = block->blockSize - newSize;
                newBlock->next = block->next;
                *prevPtr = newBlock;
                result = (uint8_t*) block;
                break;
            }
            if (block->blockSize == newSize) {
                *prevPtr = block->next;
                result = (uint8_t*) block;
                break;
            }
            prevPtr = &block->next;
        }
    }

    if (pointer && oldSize) {
        if (oldSize < sizeof(Block)) oldSize = sizeof(Block);
        Block* last = vm->freeList->first;
        while(last->next) {
            last = last->next;
        }
        if (result) {
            for (size_t i = 0; i < oldSize; i++) {
                uint8_t* src = (uint8_t*) pointer + i;
                uint8_t* dst = result + i;
                *dst = *src;
            }
        }
        Block* newBlock = (Block*) pointer;
        newBlock->next = NULL;
        newBlock->blockSize = oldSize;
        last->next = newBlock;
    }

    if (!result && newSize) {
        fprintf(stderr, "Failed to allocate memory for %zu bytes\n", newSize);
    }

    return (void*) result;
}

void collectGarbage(UNUSED VM* vm, UNUSED Compiler* compiler) {}
