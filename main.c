#include "chunk.h"
#include "vm.h"

int main(int argc, const char** argv) {
    FreeList freeList;
    initMemory(&freeList, 64 * 1024 * 1024);
    VM vm;
    initVM(&vm);
    Chunk chunk;
    initChunk(&chunk);

    writeConstant(&freeList, &chunk, 1.2, 123);
    writeConstant(&freeList, &chunk, 3.4, 123);

    writeChunk(&freeList, &chunk, OP_ADD, 123);

    writeConstant(&freeList, &chunk, 5.6, 123);
    writeChunk(&freeList, &chunk, OP_DIVIDE, 123);

    writeChunk(&freeList, &chunk, OP_NEGATE, 123);

    writeChunk(&freeList, &chunk, OP_RETURN, 123);

    interpret(&freeList, &vm, &chunk);
    freeChunk(&freeList, &chunk);
    freeVM(&freeList, &vm);
    freeMemory(&freeList);

    return 0;
}