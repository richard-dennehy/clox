#include "chunk.h"
#include "vm.h"

int main(int argc, const char** argv) {
    FreeList freeList;
    initMemory(&freeList, 64 * 1024 * 1024);
    VM vm;
    initVM(&vm);
    Chunk chunk;
    initChunk(&chunk);

    for (int i = 0; i < 257; i++) {
        writeConstant(&freeList, &chunk, i * 2, i + 1);
    }

    writeChunk(&freeList, &chunk, OP_RETURN, 258);

    interpret(&vm, &chunk);
    freeChunk(&freeList, &chunk);
    freeVM(&vm);
    freeMemory(&freeList);

    return 0;
}