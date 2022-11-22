#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "chunk.h"
#include "vm.h"

static void repl(FreeList* freeList, VM* vm) {
    assert(!"Not implemented");
}

static void runFile(FreeList* freeList, VM* vm, const char* path) {
    assert(!"Not implemented");
}

int main(int argc, const char** argv) {
    FreeList freeList;
    initMemory(&freeList, 256 * 1024 * 1024);

    VM vm;
    initVM(&vm);

    if (argc == 1) {
        repl(&freeList, &vm);
    } else if (argc == 2) {
        runFile(&freeList, &vm, argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    freeVM(&freeList, &vm);
    freeMemory(&freeList);

    return 0;
}