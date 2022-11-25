#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "chunk.h"
#include "vm.h"

static void repl(FreeList* freeList, VM* vm) {
    char line[1024];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret(freeList, vm, line);
    }
}

static void runFile(FreeList* freeList, VM* vm, const char* path) {
    FILE* file = fopen(path, "rb");
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* source = reallocate(freeList, NULL, 0, fileSize + 1);
    size_t bytesRead = fread(source, sizeof(char), fileSize, file);
    source[bytesRead] = '\0';

    fclose(file);

    InterpretResult result = interpret(freeList, vm, source);
    reallocate(freeList, source, fileSize + 1, 0);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
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