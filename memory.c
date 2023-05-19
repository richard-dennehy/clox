#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#ifdef DEBUG_LOG_GC
#include "debug.h"
#endif

#include "memory.h"
#include "object.h"

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
    vm->bytesAllocated += newSize - oldSize;
    uint8_t* result = NULL;

#ifdef DEBUG_STRESS_GC
    if (newSize > oldSize) {
        collectGarbage(vm, compiler);
    }
#else
    if (vm->bytesAllocated > vm->nextGC) {
        collectGarbage(vm, compiler);
    }
#endif

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
        while (last->next) {
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

static void blackenObject(VM* vm, Obj* object) {
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*) object);
    printValue(printf, OBJ_VAL(object));
    printf("\n");
#endif
    switch (object->type) {
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*) object;
            markObject(vm, (Obj*) function->name);
            markValueArray(vm, &function->chunk.constants);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*) object;
            markObject(vm, (Obj*) closure->function);
            for (uint32_t i = 0; i < closure->upvalueCount; i++) {
                markObject(vm, (Obj*) closure->upvalues[i]);
            }
            break;
        }
        case OBJ_UPVALUE:
            markValue(vm, ((ObjUpvalue*) object)->closed);
            break;
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
        case OBJ_NONE:
            assert(!"Use after free");
    }
}

static void traceReferences(VM* vm) {
    while (vm->greyCount) {
        Obj* object = vm->greyStack[--vm->greyCount];
        blackenObject(vm, object);
    }
}

static void sweep(VM* vm) {
    Obj* previous = NULL;
    Obj* object = vm->objects;

    while (object) {
        if (object->isMarked) {
            object->isMarked = false;
            previous = object;
            object = object->next;
        } else {
            Obj* unreached = object;
            object = object->next;

            if (previous) {
                previous->next = object;
            } else {
                vm->objects = object;
            }

            freeObject(vm, unreached);
        }
    }
}

void collectGarbage(VM* vm, Compiler* compiler) {
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = vm->bytesAllocated;
#endif

    markRoots(vm);

    if (compiler) {
        markCompilerRoots(vm, compiler);
    }

    traceReferences(vm);
    tableRemoveWhite(&vm->strings);
    sweep(vm);

    vm->nextGC = vm->bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
           before - vm->bytesAllocated, before, vm->bytesAllocated, vm->nextGC);
#endif
}
