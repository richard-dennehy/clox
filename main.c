#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "chunk.h"
#include "vm.h"

static void repl(VM* vm) {
    char line[1024];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret(vm, line);
    }
}

static void runFile(VM* vm, const char* path) {
    FILE* file = fopen(path, "rb");
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* source = reallocate(vm, NULL, NULL, 0, fileSize + 1);
    size_t bytesRead = fread(source, sizeof(char), fileSize, file);
    source[bytesRead] = '\0';

    fclose(file);

    InterpretResult result = interpret(vm, source);
    reallocate(vm, NULL, source, fileSize + 1, 0);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char** argv) {
    FreeList freeList;
    initMemory(&freeList, 256 * 1024 * 1024);

    VM vm;
    initVM(&freeList, &vm);

    if (argc == 1) {
        repl(&vm);
    } else if (argc == 2) {
        runFile(&vm, argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    freeVM(&vm);
    freeMemory(&freeList);

    return 0;
}
